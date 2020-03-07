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

///////////////////////////////////////////////////////////////////////
// Component storage interface
///////////////////////////////////////////////////////////////////////

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
	virtual bool   LoadDiff(IO::CBinaryReader& In) = 0;
	virtual bool   SaveAll(IO::CBinaryWriter& Out) const = 0;
	virtual bool   SaveDiff(IO::CBinaryWriter& Out) = 0;

	virtual void   ValidateComponents(CStrID LevelID) = 0;
	virtual void   InvalidateComponents(CStrID LevelID) = 0;
};

///////////////////////////////////////////////////////////////////////
// Default component storage with a handle array for component data and a special map for entity -> component indexing
///////////////////////////////////////////////////////////////////////

// Conditional pool member
template<typename T> struct CStoragePool { CPoolAllocator<DEM::BinaryFormat::GetMaxDiffSize<T>()> _DiffPool; };
struct CStorageNoPool {};

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
	constexpr static inline bool USE_DIFF_POOL = (DEM::BinaryFormat::GetMaxDiffSize<T>() <= 512);

	struct CIndexRecord
	{
		U64     BaseDataOffset = NO_BASE_DATA;  // Base component data can be loaded on demand when building diffs
		CHandle ComponentHandle;

		// Pre-serialized diff allows to unload objects, saving both RAM and savegame time.
		// For diffs of known small maximum size memory chunks are allocated from the pool.
		std::conditional_t<USE_DIFF_POOL, void*, Data::CBufferMalloc> BinaryDiffData = {};

		U32  DiffDataSize = 0;
		bool Deleted = false;
	};

	CInnerStorage            _Data;
	CEntityMap<CIndexRecord> _IndexByEntity;

	void ClearDiffBuffer(CIndexRecord& Record)
	{
		if constexpr (USE_DIFF_POOL)
		{
			if (Record.BinaryDiffData)
			{
				_DiffPool.Free(Record.BinaryDiffData);
				Record.BinaryDiffData = nullptr;
			}
		}
		else
		{
			Record.BinaryDiffData.Resize(0);
		}

		Record.DiffDataSize = 0;
	}
	//---------------------------------------------------------------------

	bool LoadBaseComponent(HEntity EntityID, const CIndexRecord& Record, T& Component) const
	{
		if (auto pBaseStream = _World.GetBaseStream(Record.BaseDataOffset))
		{
			// If base data is available for this component, load it (also overrides a template)
			DEM::BinaryFormat::Deserialize(IO::CBinaryReader(*pBaseStream), Component);
			return true;
		}
		else
		{
			// Load component data from the template
			// TODO: PERF - is worth it? Or always save base data / full diff (for runtime created entities)
			// instead of storing / using template ID? Will freeze created entity, changes in a template
			// will not affect it. Need profiling.
			auto pEntity = _World.GetEntity(EntityID);
			if (pEntity && pEntity->TemplateID)
			{
				_World.GetTemplateComponent<T>(pEntity->TemplateID, Component);
				return true;
			}
		}

		return false;
	}
	//---------------------------------------------------------------------

	T LoadComponent(HEntity EntityID, const CIndexRecord& Record) const
	{
		T Component;
		LoadBaseComponent(EntityID, Record, Component);

		// If diff data is available, apply it on top of base data
		if (Record.DiffDataSize)
		{
			if constexpr (USE_DIFF_POOL)
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.BinaryDiffData, MAX_DIFF_SIZE)), Component);
			else
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.BinaryDiffData.GetConstPtr(), Record.BinaryDiffData.GetSize())), Component);
		}

		return Component;
	}
	//---------------------------------------------------------------------

	void SaveComponent(HEntity EntityID, CIndexRecord& Record)
	{
		//!!!TODO: check if component is saveable and is dirty!
		//???set dirty on non-const Find()? theonly case when component data can change, plus doesn't involve per-field comparisons
		//clear dirty flag here

		n_assert2_dbg(Record.ComponentHandle, "CHandleArrayComponentStorage::SaveComponent() > call only for loaded components");

		const T& Component = _Data.GetValueUnsafe(Record.ComponentHandle)->first;

		T BaseComponent;
		const bool HasBase = LoadBaseComponent(EntityID, Record, BaseComponent);

		bool HasDiff;
		if constexpr (USE_DIFF_POOL)
		{
			if (!Record.BinaryDiffData) Record.BinaryDiffData = _DiffPool.Allocate();
			IO::CMemStream DiffStream(Record.BinaryDiffData, MAX_DIFF_SIZE);
			HasDiff = DEM::BinaryFormat::SerializeDiff(IO::CBinaryWriter(DiffStream), Component, BaseComponent);
			Record.DiffDataSize = static_cast<U32>(DiffStream.Tell());
		}
		else
		{
			// Preallocate buffer
			// TODO: vector-like allocation strategy inside a CBufferMalloc?
			if (!Record.BinaryDiffData.GetSize()) Record.BinaryDiffData.Resize(512);
			IO::CMemStream DiffStream(Record.BinaryDiffData);
			HasDiff = DEM::BinaryFormat::SerializeDiff(IO::CBinaryWriter(DiffStream), Component, BaseComponent);
			Record.DiffDataSize = static_cast<U32>(DiffStream.Tell());
		}

		if (HasBase && !HasDiff)
		{
			// No diff against the base, component is unchanged, nothing must be saved
			ClearDiffBuffer(Record);
		}
		else
		{
			// When HasDiff is false, the component is new but equal to the default one, and we
			// must save empty diff, but not nothing. It is already written in SerializeDiff.
			if constexpr (!USE_DIFF_POOL)
			{
				// Truncate if too many unused bytes left
				if (Record.BinaryDiffData.GetSize() - Record.DiffDataSize > 400)
					Record.BinaryDiffData.Resize(Record.DiffDataSize);
			}
		}
	}
	//---------------------------------------------------------------------

	bool IsComponentEqualToTemplate(HEntity EntityID, const T& Component) const
	{
		auto pEntity = _World.GetEntity(EntityID);
		if (pEntity && pEntity->TemplateID)
		{
			T TplComponent;
			if (_World.GetTemplateComponent<T>(pEntity->TemplateID, TplComponent) && Component == TplComponent)
				return true;
		}

		return false;
	}
	//---------------------------------------------------------------------

	// NB: can't embed into lambdas, both branches of if constexpr are compiled for some reason
	static inline void WriteComponentDiff(IO::CBinaryWriter& Out, HEntity EntityID, const CIndexRecord& Record)
	{
		if (Record.DiffDataSize)
		{
			Out.Write(EntityID.Raw);
			Out.Write(Record.DiffDataSize);
			if constexpr (USE_DIFF_POOL)
				Out.GetStream().Write(Record.BinaryDiffData, Record.DiffDataSize);
			else
				Out.GetStream().Write(Record.BinaryDiffData.GetConstPtr(), Record.DiffDataSize);
		}
	}
	//---------------------------------------------------------------------

public:

	CHandleArrayComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
		, _Data(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
	{
		n_assert_dbg(InitialCapacity <= CInnerStorage::MAX_CAPACITY);
	}

	~CHandleArrayComponentStorage() { if constexpr (USE_DIFF_POOL) _DiffPool.Clear(); }

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
		_IndexByEntity.emplace(EntityID, CIndexRecord{ NO_BASE_DATA, Handle, {} });
		return &pPair->first;
	}
	//---------------------------------------------------------------------

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
	//---------------------------------------------------------------------

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || !It->Value.ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first;
	}
	//---------------------------------------------------------------------

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || !It->Value.ComponentHandle) return nullptr;

		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		return &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first;
	}
	//---------------------------------------------------------------------

	virtual bool RemoveComponent(HEntity EntityID) override { return Remove(EntityID); }
	//---------------------------------------------------------------------

	bool DeserializeComponentFromParams(T& Out, const Data::CData& In) const
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		DEM::ParamsFormat::Deserialize(In, Out);
		return true;
	}
	//---------------------------------------------------------------------

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
	//---------------------------------------------------------------------

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		if (auto pComponent = Find(EntityID))
		{
			if (IsComponentEqualToTemplate(EntityID, *pComponent)) return false;
			DEM::ParamsFormat::Serialize(Out, *pComponent);
			return true;
		}

		return false;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;
		const auto& IndexRecord = It->Value;

		if (IndexRecord.Deleted)
		{
			// Explicitly save as deleted
			Out = Data::CData();
			return true;
		}
		else
		{
			T BaseComponent;
			LoadBaseComponent(EntityID, IndexRecord, BaseComponent);

			if (IndexRecord.ComponentHandle)
			{
				const T& Component = _Data.GetValueUnsafe(IndexRecord.ComponentHandle)->first;
				return DEM::ParamsFormat::SerializeDiff(Out, Component, BaseComponent);
			}
			else
			{
				T Component = BaseComponent;

				if (IndexRecord.DiffDataSize)
				{
					if constexpr (USE_DIFF_POOL)
						DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(IndexRecord.BinaryDiffData, MAX_DIFF_SIZE)), Component);
					else
						DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(IndexRecord.BinaryDiffData.GetConstPtr(), IndexRecord.BinaryDiffData.GetSize())), Component);
				}

				return DEM::ParamsFormat::SerializeDiff(Out, Component, BaseComponent);
			}
		}
	}
	//---------------------------------------------------------------------

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
			const auto EntityIDRaw = In.Read<decltype(HEntity::Raw)>();
			const auto OffsetInBase = In.Read<U64>();
			_IndexByEntity.emplace(HEntity{ EntityIDRaw }, CIndexRecord{ OffsetInBase, CInnerStorage::INVALID_HANDLE, {} });
		}

		auto ComponentDataSkipOffset = In.Read<U64>();
		In.GetStream().Seek(static_cast<I64>(ComponentDataSkipOffset), IO::ESeekOrigin::Seek_Begin);

		return true;
	}
	//---------------------------------------------------------------------

	// TODO: entity templates in Record.BaseDataOffset == NO_BASE_DATA
	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		// Instead of cleaning all up, we only clean old diff data and current states affected by it

		std::vector<HEntity> RecordsToDelete;
		RecordsToDelete.reserve(_Data.size() / 4);
		_IndexByEntity.ForEach([this, &RecordsToDelete](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.BaseDataOffset == NO_BASE_DATA)
			{
				// Components without base and template were created in runtime and must be deleted entirely
				RecordsToDelete.push_back(EntityID);
			}
			else if (Record.DiffDataSize || Record.Deleted)
			{
				// Old diff info must be erased, components deleted in runtime must be restored
				ClearDiffBuffer(Record);
				Record.Deleted = false;
			}
			else
			{
				// This record is already in a base state, can avoid recreation
				return;
			}

			// For all records affected a current state becomes inactual and must be unloaded
			if (Record.ComponentHandle)
			{
				_Data.Free(Record.ComponentHandle);
				Record.ComponentHandle = CInnerStorage::INVALID_HANDLE;
			}
		});

		for (auto EntityID : RecordsToDelete)
			_IndexByEntity.erase(EntityID);

		// Load deleted component list, mark corresponding records as deleted

		HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			if (auto It = _IndexByEntity.find(EntityID))
				It->Value.Deleted = true;
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		// Load diff data for added and modified components

		EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			auto It = _IndexByEntity.find(EntityID);
			if (!It) It = _IndexByEntity.emplace(EntityID, CIndexRecord{ NO_BASE_DATA, CInnerStorage::INVALID_HANDLE, {} });
			auto& IndexRecord = It->Value;

			In.Read(IndexRecord.DiffDataSize);

			if constexpr (USE_DIFF_POOL)
			{
				if (!IndexRecord.BinaryDiffData) IndexRecord.BinaryDiffData = _DiffPool.Allocate();
				In.GetStream().Read(IndexRecord.BinaryDiffData, IndexRecord.DiffDataSize);
			}
			else
			{
				IndexRecord.BinaryDiffData.Resize(IndexRecord.DiffDataSize);
				In.GetStream().Read(IndexRecord.BinaryDiffData.GetPtr(), IndexRecord.DiffDataSize);
			}

			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		return true;
	}
	//---------------------------------------------------------------------

	// This method is primarily suited for saving levels in the editor, not for ingame use. Use SaveDiff to save game.
	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		std::vector<std::pair<Data::CBufferMalloc, U64>> BinaryData;
		std::vector<std::pair<HEntity, size_t>> EntityToData;
		BinaryData.reserve(_IndexByEntity.size());
		EntityToData.reserve(_IndexByEntity.size());

		// Choose some reasonable initial size for the buffer to minimize reallocations.
		// Max diff size is always more than max whole data size due to field IDs, so it is safe to choose it.
		IO::CMemStream IntermediateStream(std::min<decltype(MAX_DIFF_SIZE)>(MAX_DIFF_SIZE, 512));
		IO::CBinaryWriter Intermediate(IntermediateStream);

		_IndexByEntity.ForEach([&](HEntity EntityID, const CIndexRecord& Record)
		{
			Intermediate.GetStream().Seek(0, IO::Seek_Begin);

			if (Record.ComponentHandle)
			{
				const T& Component = _Data.GetValueUnsafe(Record.ComponentHandle)->first;
				if (!IsComponentEqualToTemplate(EntityID, Component))
					DEM::BinaryFormat::Serialize(Intermediate, Component);
			}
			else if (!Record.Deleted)
			{
				T Component = LoadComponent(EntityID, Record);
				if (!IsComponentEqualToTemplate(EntityID, Component))
					DEM::BinaryFormat::Serialize(Intermediate, Component);
			}

			const auto SerializedSize = static_cast<UPTR>(Intermediate.GetStream().Tell());
			if (!SerializedSize) return;

			const void* pComponentData = Intermediate.GetStream().Map();

			//???TODO: use hashes for faster comparison?
			for (size_t i = 0; i < BinaryData.size(); ++i)
				if (!BinaryData[i].first.Compare(pComponentData, SerializedSize))
				{
					EntityToData.emplace_back(EntityID, i);
					return;
				}

			Data::CBufferMalloc NewBuffer(SerializedSize);
			memcpy(NewBuffer.GetPtr(), pComponentData, SerializedSize);
			BinaryData.emplace_back(std::move(NewBuffer), 0);

			EntityToData.emplace_back(EntityID, BinaryData.size() - 1);
		});

		// Base offset of component data is current offset plus size of indexing table, written just below
		U64 ComponentDataOffset = Out.GetStream().Tell() +
			sizeof(U32) +
			EntityToData.size() * (sizeof(decltype(HEntity::Raw)) + sizeof(U64)) +
			sizeof(U64);

		for (auto& [Buffer, Offset] : BinaryData)
		{
			Offset = ComponentDataOffset;
			ComponentDataOffset += Buffer.GetSize();
		}

		// Save index table of EntityID -> component data offset
		Out.Write(static_cast<U32>(EntityToData.size()));
		for (const auto& Record : EntityToData)
		{
			Out.Write(Record.first.Raw);
			Out.Write(BinaryData[Record.second].second);
		}

		// Save component data skip offset
		Out.Write(ComponentDataOffset);

		// Store component data
		for (const auto& [Buffer, Offset] : BinaryData)
		{
			// Ensure our calculations are correct
			n_assert_dbg(Out.GetStream().Tell() == Offset);
			Out.GetStream().Write(Buffer.GetConstPtr(), Buffer.GetSize());
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveDiff(IO::CBinaryWriter& Out) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		// Save the list of deleted components

		_IndexByEntity.ForEach([this, &Out](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.Deleted) Out.Write(EntityID.Raw);
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		// Save diff data of added and modified components

		_IndexByEntity.ForEach([this, &Out](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.ComponentHandle) SaveComponent(EntityID, Record);
			WriteComponentDiff(Out, EntityID, Record);
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}
	//---------------------------------------------------------------------

	virtual void ValidateComponents(CStrID LevelID) override
	{
		_IndexByEntity.ForEach([this, LevelID](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.ComponentHandle || Record.Deleted)
			{
				n_assert_dbg(!Record.ComponentHandle || !Record.Deleted);
				return;
			}

			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			Record.ComponentHandle = _Data.Allocate({ std::move(LoadComponent(EntityID, Record)), EntityID });
		});
	}
	//---------------------------------------------------------------------

	virtual void InvalidateComponents(CStrID LevelID) override
	{
		_IndexByEntity.ForEach([this, LevelID](HEntity EntityID, CIndexRecord& Record)
		{
			if (!Record.ComponentHandle) return;

			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			SaveComponent(EntityID, Record);

			_Data.Free(Record.ComponentHandle);
			Record.ComponentHandle = CInnerStorage::INVALID_HANDLE;
		});
	}
	//---------------------------------------------------------------------

	auto begin() { return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }
};

///////////////////////////////////////////////////////////////////////
// Default storage for empty components (flags)
///////////////////////////////////////////////////////////////////////

template<typename T>
class CEmptyComponentStorage : public IComponentStorage
{
protected:

	std::set<HEntity> _Base;
	std::set<HEntity> _Actual;
	T                 _SharedInstance; // It is enough to have one instance of component without data

public:

	CEmptyComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
	{
		// unordered_set has ::reserve(), custom pooled set may have it to, or use std::set with pool allocator?
	}

	DEM_FORCE_INLINE T* Add(HEntity EntityID)
	{
		_Actual.insert(EntityID);
		return &_SharedInstance;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE bool Remove(HEntity EntityID)
	{
		return _Actual.erase(EntityID) > 0;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _Actual.find(EntityID);
		return (It == _Actual.cend()) ? nullptr : &_SharedInstance;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _Actual.find(EntityID);
		return (It == _Actual.cend()) ? nullptr : &_SharedInstance;
	}
	//---------------------------------------------------------------------

	virtual bool RemoveComponent(HEntity EntityID) override { return Remove(EntityID); }
	//---------------------------------------------------------------------

	bool DeserializeComponentFromParams(T& Out, const Data::CData& In) const
	{
		// No data to deserialize, Out always is up to date, return value is used for control
		if (auto pBoolData = pData->As<bool>()) return *pBoolData;
		else if (auto pParamsData = pData->As<Data::PParams>()) return (*pParamsData).IsValidPtr();
		return false;
	}
	//---------------------------------------------------------------------

	virtual bool LoadComponentFromParams(HEntity EntityID, const Data::CData& In) override
	{
		// Support bool 'true' or section (empty is enough)
		if (auto pBoolData = In.As<bool>())
		{
			if (!*pBoolData) return false;
		}
		else if (auto pParamsData = In.As<Data::PParams>())
		{
			if (!*pParamsData) return false;
		}
		else
		{
			return false;
		}

		Add(EntityID);
		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if (_Actual.find(EntityID) == _Actual.cend()) return false;
		Out = true;
		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		const bool HasBase = (_Base.find(EntityID) != _Base.cend());
		const bool HasActual = (_Actual.find(EntityID) != _Actual.cend());
		if (HasBase == HasActual) return false;

		if (HasBase) Out = Data::CData();
		else Out = true;

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		_Base.clear();
		_Actual.clear();

		const auto Count = In.Read<U32>();
		for (U32 i = 0; i < Count; ++i)
		{
			const HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
			_Base.insert(EntityID);
			_Actual.insert(EntityID);
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		_Actual = _Base;

		HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			_Actual.erase(EntityID);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			_Actual.insert(EntityID);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		Out.Write(static_cast<U32>(_Actual.size()));
		for (HEntity EntityID : _Actual)
			Out.Write(EntityID.Raw);
		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveDiff(IO::CBinaryWriter& Out) override
	{
		// Save the list of deleted components

		for (HEntity EntityID : _Base)
			if (_Actual.find(EntityID) == _Actual.cend())
				Out << EntityID.Raw;

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		// Save the list of added components

		for (HEntity EntityID : _Actual)
			if (_Base.find(EntityID) == _Base.cend())
				Out << EntityID.Raw;

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}
	//---------------------------------------------------------------------

	virtual void ValidateComponents(CStrID LevelID) override {}
	virtual void InvalidateComponents(CStrID LevelID) override {}

	//auto begin() { return _Data.begin(); }
	//auto begin() const { return _Data.begin(); }
	//auto cbegin() const { return _Data.cbegin(); }
	//auto end() { return _Data.end(); }
	//auto end() const { return _Data.end(); }
	//auto cend() const { return _Data.cend(); }
};

///////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////

template<typename T>
struct TComponentTraits
{
	using TStorage = std::conditional_t<std::is_empty_v<T>, CEmptyComponentStorage<T>, CHandleArrayComponentStorage<T>>;
};

template<typename T> using TComponentStoragePtr = typename TComponentTraits<just_type_t<T>>::TStorage*;

}
