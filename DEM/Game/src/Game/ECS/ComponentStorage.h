#pragma once
#include <Game/ECS/EntityMap.h>
#include <Data/SerializeToParams.h>
#include <Data/SerializeToBinary.h>

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

// Default component storage with a handle array for component data and a special map for entity -> component indexing
template<typename T, typename H = uint32_t, size_t IndexBits = 18, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage
{
public:

	using CPair = std::pair<T, HEntity>;
	using CInnerStorage = Data::CHandleArray<CPair, H, IndexBits, ResetOnOverflow>;
	using CHandle = typename CInnerStorage::CHandle;

protected:

	constexpr static inline size_t NO_BASE_DATA = std::numeric_limits<size_t>().max();

	struct CIndexRecord
	{
		CHandle ComponentHandle;
		size_t  BaseDataOffset = NO_BASE_DATA;  // Base component data can be loaded on demand when building diffs
		// binary diff data fixed or dynamic // Pre-serialized diff allows to unload objects, saving both RAM and savegame time
		// FIXME: for now:
		void* pBinaryDiffData = nullptr;
	};

	CInnerStorage            _Data;
	CEntityMap<CIndexRecord> _IndexByEntity;

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
		auto It = _IndexByEntity.find(EntityID);
		if (It != _IndexByEntity.cend())
			return It->ComponentHandle ? &_Data.GetValueUnsafe(It->ComponentHandle)->first : nullptr;

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
		if (It == _IndexByEntity.cend()) FAIL;

		if (It->ComponentHandle) _Data.Free(It->ComponentHandle);

		if (It->BaseDataOffset == NO_BASE_DATA)
		{
			_IndexByEntity.erase(It);
		}
		else
		{
			// write 'deleted' diff/flag
		}

		OK;
	}

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (It == _IndexByEntity.cend() || !It->ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->ComponentHandle)->first;
	}

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		if (It == _IndexByEntity.cend() || !It->ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->ComponentHandle)->first;
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

		// FIXME: load base data if offset is valid (_World.GetBaseReader(Offset) / _World.GetBaseDataPtr(Offset))
		T* pBaseComponent = nullptr;

		if (auto pComponent = Find(EntityID))
		{
			return DEM::ParamsFormat::SerializeDiff(Out, *pComponent, pBaseComponent ? *pBaseComponent : T{});
		}
		else if (pBaseComponent)
		{
			// Explicitly deleted
			Out = Data::CData();
			return true;
		}
		return false;
	}

	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		//erase all previous data

		// register EntityID -> { HComponent = invalid, OffsetInBase, DiffData = nullptr }, don't Deserialize components
		// registry could be a sparse set, now it is a CEntityMap
		//!!!for reuse of identical data EntityID->Offset index must be prebuilt! Write without merging first, then add.
		// In this case we can restore mapping by adding a component full data size (constexpr from meta? or saved in file?).
		// We could load base component data right here but we don't need it in runtime, so load on demand to the stack.

		//!!!separate function must exist to restore actual components from base data pointer and optional diff!
		// When unloading the level, may delete component and write binary diff to the field or may keep actual component
		// and build diff from it each save. Caching diff reduces base reading too! If can detect component changes
		// (at least getting mutable value ref by handle), could skip updating diff. Or explicit 'commit/save' to mark changed?
		// So, saving binary diffs:
		// + allows to save diff once for unloaded objects or objects that didnt change
		// + allows to read base component data only at diff update, not every save
		// + speedup saves by writing ready pieces of binary data
		// - require binary storage, sometimes of size unknown at the compile time (cant use pool of ready made chunks)
		// - don't support HRD diffs

		const auto ComponentCount = In.Read<uint32_t>();
		for (uint32_t i = 0; i < ComponentCount; ++i)
		{
			const auto EntityIDRaw = In.Read<CInnerStorage::THandleValue>();
			if (auto pComponent = Add({ EntityIDRaw }))
				DEM::BinaryFormat::Deserialize(In, *pComponent);
		}

		return true;
	}

	//!!!TODO: merge identical (compare bytes AFTER serialization)! Optional flag?
	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		Out.Write(static_cast<uint32_t>(_Data.size()));
		for (const auto& [Component, EntityID] : _Data)
		{
			// save actual component (if not loaded, restore right now for writing)
			Out.Write(EntityID.Raw);
			DEM::BinaryFormat::Serialize(Out, Component);
		}

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
