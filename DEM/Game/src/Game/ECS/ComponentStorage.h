#pragma once
#include <Game/ECS/EntityMap.h>
#include <Data/SerializeToParams.h>
#include <Data/SerializeToBinary.h>
#include <Data/Buffer.h>
#include <IO/Streams/MemStream.h>
#include <System/Allocators/PoolAllocator.h>

// Stores components of specified type. Each component type can have only one storage type.
// Interface is provided for the game world to handle all storages transparently. In places
// where the component type is known it is preferable to use static storage type.
// TComponentTraits determine what type of storage which component type uses.
// Effective and compact handle array storage is used by default.

namespace DEM::Game
{
class CGameWorld;
typedef std::unique_ptr<class IComponentStorage> PComponentStorage;

class IComponentStorage
{
protected:

	const CGameWorld& _World;

public:

	IComponentStorage(const CGameWorld& World) : _World(World) {}
	virtual ~IComponentStorage() = default;

	virtual bool   RemoveComponent(HEntity EntityID) = 0;

	virtual bool   LoadComponentFromParams(HEntity EntityID, const Data::CData& In) = 0;
	virtual bool   SaveComponentToParams(HEntity EntityID, Data::CData& Out) const = 0;
	virtual bool   SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const = 0;
	virtual bool   LoadBase(IO::CBinaryReader& In) = 0;
	virtual bool   SaveAll(IO::CBinaryWriter& Out) const = 0;
	virtual bool   SaveDiff(IO::CBinaryWriter& Out) const = 0;
};

// Conditional pool member for CHandleArrayComponentStorage
template<typename T> struct CStoragePool { CPoolAllocator<DEM::BinaryFormat::GetMaxDiffSize<T>()> _Pool; };
struct CStorageNoPool {};

// Default component storage with a handle array for component data and a special map for entity -> component indexing
template<typename T, typename H = uint32_t, size_t IndexBits = 18, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage,
	std::conditional_t<DEM::BinaryFormat::GetMaxDiffSize<T>() <= 512, CStoragePool<T>, CStorageNoPool>
{
public:

	using CPair = std::pair<T, HEntity>;
	using CInnerStorage = Data::CHandleArray<CPair, H, IndexBits, ResetOnOverflow>;
	using CHandle = typename CInnerStorage::CHandle;

protected:

	constexpr static inline auto NO_BASE_DATA = std::numeric_limits<U64>().max();
	constexpr static inline auto MAX_DIFF_SIZE = DEM::BinaryFormat::GetMaxDiffSize<T>();

	struct CIndexRecord
	{
		CHandle ComponentHandle;
		U64     BaseDataOffset = NO_BASE_DATA;  // Base component data can be loaded on demand when building diffs

		// Pre-serialized diff allows to unload objects, saving both RAM and savegame time.
		// For diffs of known small maximum size memory chunks are allocated from the pool.
		std::conditional_t<
			MAX_DIFF_SIZE <= 512,
			void*,
			Data::PBuffer> BinaryDiffData;

		bool Deleted = false;
	};

	CInnerStorage            _Data;
	CEntityMap<CIndexRecord> _IndexByEntity;

	void ClearDiffBuffer(CIndexRecord& Record)
	{
		if constexpr (DEM::BinaryFormat::GetMaxDiffSize<T>() <= 512)
			if (Record.BinaryDiffData)
				_Pool.Free(Record.BinaryDiffData);

		Record.BinaryDiffData = nullptr;
	}

	T LoadComponent(const CIndexRecord& Record) const
	{
		T Component;

		// If base data is available for this component, load it
		if (auto pBaseStream = _World.GetBaseStream(Record.BaseDataOffset))
			DEM::BinaryFormat::Deserialize(IO::CBinaryReader(*pBaseStream), Component);

		// If diff data is available, apply it on top of base data
		if (Record.BinaryDiffData)
		{
			if constexpr (MAX_DIFF_SIZE <= 512)
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.BinaryDiffData, MAX_DIFF_SIZE)), Component, Component);
			else
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(*Record.BinaryDiffData)), Component, Component);
		}

		return Component;
	}

public:

	CHandleArrayComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
		, _Data(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
	{
		n_assert_dbg(InitialCapacity <= CInnerStorage::MAX_CAPACITY);
	}

	// TODO: describe as a static interface part
	// TODO: pass optional precreated component inside?
	T* Add(HEntity EntityID)
	{
		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		if (auto It = _IndexByEntity.find(EntityID))
			return It->Value.ComponentHandle ? &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first : nullptr;

		auto Handle = _Data.Allocate();
		CPair* pPair = _Data.GetValueUnsafe(Handle);
		if (!pPair) return nullptr;

		pPair->second = EntityID;
		_IndexByEntity.emplace(EntityID, { Handle, NO_BASE_DATA, nullptr });
		return &pPair->first;
	}

	// TODO: describe as a static interface part
	bool Remove(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) FAIL;
		auto& IndexRecord = It->Value;

		if (IndexRecord.ComponentHandle)
		{
			_Data.Free(IndexRecord.ComponentHandle);
			IndexRecord.ComponentHandle = CInnerStorage::INVALID_HANDLE;
		}

		if (IndexRecord.BaseDataOffset == NO_BASE_DATA)
		{
			// Component not present in a base can be deleted without a trace
			_IndexByEntity.erase(It);
		}
		else
		{
			IndexRecord.Deleted = true;
			ClearDiffBuffer(IndexRecord);
		}

		OK;
	}

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || !It->Value.ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first;
	}

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || !It->Value.ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first;
	}

	virtual bool RemoveComponent(HEntity EntityID) override { return Remove(EntityID); }

	virtual bool LoadComponentFromParams(HEntity EntityID, const Data::CData& In) override
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			auto pComponent = Find(EntityID);
			if (!pComponent) pComponent = Add(EntityID);
			if (pComponent)
			{
				DEM::ParamsFormat::Deserialize(In, *pComponent);
				return true;
			}
		}
		return false;
	}

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
			if (auto pComponent = Find(EntityID))
			{
				DEM::ParamsFormat::Serialize(Out, *pComponent);
				return true;
			}
		return false;
	}

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;
		const auto& IndexRecord = It->Value;

		if (IndexRecord.Deleted)
		{
			// Explicitly deleted
			Out = Data::CData();
			return true;
		}
		else
		{
			//???FIXME: or read binary diff into a temporary stack object?
			if (!IndexRecord.ComponentHandle) return false;

			const T& Component = _Data.GetValueUnsafe(IndexRecord.ComponentHandle)->first;

			T BaseComponent;
			if (auto pBaseStream = _World.GetBaseStream(IndexRecord.BaseDataOffset))
				DEM::BinaryFormat::Deserialize(IO::CBinaryReader(*pBaseStream), BaseComponent);

			return DEM::ParamsFormat::SerializeDiff(Out, Component, BaseComponent);
		}

		return false;
	}

	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		// Erase all existing data, both base and diff
		for (auto& IndexRecord : _IndexByEntity)
			ClearDiffBuffer(IndexRecord);
		_IndexByEntity.clear();
		_Data.Clear();

		// We could create base components from data right here, but we will deserialize them on demand instead
		const auto Count = In.Read<U32>();
		for (U32 i = 0; i < Count; ++i)
		{
			const auto EntityIDRaw = In.Read<CInnerStorage::THandleValue>();
			const auto OffsetInBase = In.Read<U64>();
			_IndexByEntity.emplace(HEntity{ EntityIDRaw }, { CInnerStorage::INVALID_HANDLE, OffsetInBase, nullptr });
		}

		//!!!separate function must exist to restore actual components from base data pointer and optional diff!
		// DEM::BinaryFormat::Deserialize(In, *pComponent);
		// When unloading the level, may delete component and write binary diff to the field or may keep actual component
		// and build diff from it each save. Caching diff reduces base reading too! If can detect component changes
		// (at least getting mutable value ref by handle), could skip updating diff. Or explicit 'commit/save' to mark changed?
		// So, saving binary diffs:
		// + allows to save diff once for unloaded objects or objects that didnt change
		// + allows to read base component data only at diff update, not every save
		// + speedup saves by writing ready pieces of binary data
		// - require binary storage, sometimes of size unknown at the compile time (cant use pool of ready made chunks)
		// - don't support HRD diffs

		return true;
	}

	// This method is primarily suited for saving levels in the editor, not for ingame use. Use SaveDiff to save game.
	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		std::vector<std::pair<Data::CBufferMalloc, U64>> BinaryData;
		std::vector<std::pair<HEntity, size_t>> EntityToData;
		BinaryData.reserve(_IndexByEntity.size());
		EntityToData.reserve(_IndexByEntity.size());

		// Choose some reasonable initial size of the buffer to minimize reallocations
		IO::CBinaryWriter Intermediate(IO::CMemStream(std::min<decltype(MAX_DIFF_SIZE)>(MAX_DIFF_SIZE, 512)));

		_IndexByEntity.ForEach([this, &Intermediate](HEntity EntityID, const CIndexRecord& Record)
		{
			Intermediate.GetStream().Seek(0, IO::Seek_Begin);

			if (Record.ComponentHandle)
				DEM::BinaryFormat::Serialize(Intermediate, _Data.GetValueUnsafe(Record.ComponentHandle)->first);
			else if (!Record.Deleted)
				DEM::BinaryFormat::Serialize(Intermediate, LoadComponent(Record));
			else
				return;

			const void* pComponentData = Intermediate.GetStream().Map();
			const auto SerializedSize = Intermediate.GetStream().Tell();

			//???use hashes for faster comparison?
			for (size_t i = 0; i < BinaryData.size(); ++i)
				if (!BinaryData[i].Compare(pComponentData, SerializedSize))
				{
					EntityToData.emplace_back(EntityID, i);
					return;
				}

			//!!!calculate offsets!

			Data::CBufferMalloc NewBuffer(SerializedSize);
			memcpy(NewBuffer.GetPtr(), pComponentData, SerializedSize);
			BinaryData.emplace_back(std::move(NewBuffer), 0);

			EntityToData.emplace_back(EntityID, BinaryData.size() - 1);
		});

		// Save index EntityID -> component data offset
		Out.Write(static_cast<uint32_t>(EntityToData.size()));
		for (const auto& Record : EntityToData)
		{
			Out.Write(Record.first.Raw);
			Out.Write(BinaryData[Record.second].second);
		}

		// Store component data
		for (const auto& Record : BinaryData)
			Out.GetStream().Write(Record.first.GetConstPtr(), Record.first.GetSize());

		return true;
	}

	// update diffs of all loaded entities, write to Out
	virtual bool SaveDiff(IO::CBinaryWriter& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		//   per component
		//     if component is data-less, just write a list of entities with deleted component, then with added
		//     if component is loaded, update diff: load base component, write diff current / base
		//     if modified, diff against base, if new, diff against default-created object
		//     if component is really deleted, not just 'not loaded', write special 'deleted' diff flag
		//     save diff data or 'deleted' flag to the output
		//     could write deleted components first, then added/modified

		for (const auto& Component : _Data)
		{
			// if (Entity.Level != pLevel) continue;
			//!!!save diff!
		}

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}

	//LoadDiff - delete diff data and objects without base, unload components for affected recs, cache new diff binary data, (reload unloaded??? optional?)
	//!!!if diff data is 'deleted', don't load component! may even force-unload it.

	//???all entities in a level? or different functions for single entity and for level?
	//!!!first create what will be used right now!
	//LoadEntity - skip if no record, create base/T(), apply diff on it if present, register and return resulting component, if diff is 'deleted' simply destroy component if loaded and return null
	//UnloadEntity - SaveEntity for what is unloaded (see skips there too!), delete component from storage, clear handle in record
	//SaveEntity - skip if no record, not loaded, not saveable or not dirty, get base/T(), calc diff against it and write to diff cache

	auto begin() { return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }
};

template<class T>
struct TComponentTraits
{
	using TStorage = CHandleArrayComponentStorage<T>;
	using THandle = typename TStorage::CInnerStorage::CHandle;
};

template<typename T> using TComponentStoragePtr = typename TComponentTraits<just_type_t<T>>::TStorage*;

}
