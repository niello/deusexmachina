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

enum class EComponentState : U8
{
	NoBase,    // Component is created at runtime (used only for base state)
	Templated, // Component is loaded from the template with optional per-instance diff
	Explicit,  // Component is explicitly added, template will be ignored
	Deleted    // Component is explicitly deleted, template will be ignored
};

class IComponentStorage
{
protected:

	const CGameWorld& _World;

public:

	IComponentStorage(const CGameWorld& World) : _World(World) {}
	virtual ~IComponentStorage() = default;

	virtual bool   RemoveComponent(HEntity EntityID) = 0;

	virtual bool   LoadComponentFromParams(HEntity EntityID, const Data::CData& In, bool Replace) = 0;
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
template<typename T> constexpr bool STORAGE_USE_DIFF_POOL = (DEM::BinaryFormat::GetMaxDiffSize<T>() <= 512);
template<typename T> struct CStoragePool { CPoolAllocator<DEM::BinaryFormat::GetMaxDiffSize<T>()> _DiffPool; };
struct CStorageNoPool {};

template<typename T, typename H = uint32_t, size_t IndexBits = 18, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage,
	std::conditional_t<STORAGE_USE_DIFF_POOL<T>, CStoragePool<T>, CStorageNoPool>
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
		// Pre-serialized diff allows to unload objects, saving both RAM and savegame time.
		// For diffs of known small maximum size memory chunks are allocated from the pool.
		using TDiffData = std::conditional_t<STORAGE_USE_DIFF_POOL<T>, void*, Data::CBufferMalloc>;

		U64             BaseDataOffset = NO_BASE_DATA;       // For on-demand base data loading, read-only
		TDiffData       DiffData = {};                       // Diff between base and current components
		U32             DiffDataSize = 0;
		CHandle         ComponentHandle;
		EComponentState State = EComponentState::Templated;
		EComponentState BaseState = EComponentState::NoBase; // For on-demand base data loading, read-only
	};

	CInnerStorage            _Data;
	CEntityMap<CIndexRecord> _IndexByEntity;

	void ClearDiffBuffer(CIndexRecord& Record)
	{
		if constexpr (STORAGE_USE_DIFF_POOL<T>)
		{
			if (Record.DiffData)
			{
				_DiffPool.Free(Record.DiffData);
				Record.DiffData = nullptr;
			}
		}
		else
		{
			Record.DiffData.Resize(0);
		}

		Record.DiffDataSize = 0;
	}
	//---------------------------------------------------------------------

	// Restore the component instace from base data
	bool LoadBaseComponent(HEntity EntityID, const CIndexRecord& Record, T& Component) const
	{
		if (Record.BaseState == EComponentState::Explicit)
		{
			// If base data is available for this component, load it (overrides a template)
			if (auto pBaseStream = _World.GetBaseStream(Record.BaseDataOffset))
			{
				DEM::BinaryFormat::Deserialize(IO::CBinaryReader(*pBaseStream), Component);
				return true;
			}
		}
		else if (Record.BaseState == EComponentState::Templated)
		{
			// Get the template and apply an optional diff on top of it
			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || !_World.GetTemplateComponent<T>(pEntity->TemplateID, Component)) return false;
			if (auto pBaseStream = _World.GetBaseStream(Record.BaseDataOffset))
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(*pBaseStream), Component);
			return true;
		}
		else if (Record.State == EComponentState::Templated)
		{
			// Runtime-created template components base on templates
			auto pEntity = _World.GetEntity(EntityID);
			return pEntity && _World.GetTemplateComponent<T>(pEntity->TemplateID, Component);
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
			if constexpr (STORAGE_USE_DIFF_POOL<T>)
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData, MAX_DIFF_SIZE)), Component);
			else
				DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData.GetConstPtr(), Record.DiffData.GetSize())), Component);
		}

		return Component;
	}
	//---------------------------------------------------------------------

	// Converts a component to binary diff against the base
	void SaveComponent(HEntity EntityID, CIndexRecord& Record)
	{
		//!!!TODO: check if component is saveable and is dirty!
		//???set dirty on non-const Find()? the only case when component data can change, plus doesn't involve per-field comparisons
		//clear dirty flag here

		n_assert2_dbg(Record.ComponentHandle, "CHandleArrayComponentStorage::SaveComponent() > call only for loaded components");

		const T& Component = _Data.GetValueUnsafe(Record.ComponentHandle)->first;

		T BaseComponent;
		const bool HasBase = LoadBaseComponent(EntityID, Record, BaseComponent);

		bool HasDiff;
		if constexpr (STORAGE_USE_DIFF_POOL<T>)
		{
			if (!Record.DiffData) Record.DiffData = _DiffPool.Allocate();
			IO::CMemStream DiffStream(Record.DiffData, MAX_DIFF_SIZE);
			HasDiff = DEM::BinaryFormat::SerializeDiff(IO::CBinaryWriter(DiffStream), Component, BaseComponent);
			Record.DiffDataSize = static_cast<U32>(DiffStream.Tell());
		}
		else
		{
			// Preallocate buffer
			// TODO: vector-like allocation strategy inside a CBufferMalloc?
			if (!Record.DiffData.GetSize()) Record.DiffData.Resize(512);
			IO::CMemStream DiffStream(Record.DiffData);
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
			if constexpr (!STORAGE_USE_DIFF_POOL<T>)
			{
				// Truncate if too many unused bytes left
				if (Record.DiffData.GetSize() - Record.DiffDataSize > 400)
					Record.DiffData.Resize(Record.DiffDataSize);
			}
		}
	}
	//---------------------------------------------------------------------

	// NB: can't embed into lambdas, both branches of if constexpr are compiled for some reason
	static inline void WriteComponentDiff(IO::CBinaryWriter& Out, HEntity EntityID, const CIndexRecord& Record)
	{
		if (Record.DiffDataSize)
		{
			Out.Write(EntityID.Raw);
			Out.Write(Record.DiffDataSize);
			if constexpr (STORAGE_USE_DIFF_POOL<T>)
				Out.GetStream().Write(Record.DiffData, Record.DiffDataSize);
			else
				Out.GetStream().Write(Record.DiffData.GetConstPtr(), Record.DiffDataSize);
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

	virtual ~CHandleArrayComponentStorage() override { if constexpr (STORAGE_USE_DIFF_POOL<T>) _DiffPool.Clear(); }

	// TODO: describe as a static interface part
	// TODO: pass optional precreated component inside?
	T* Add(HEntity EntityID)
	{
		CHandle Handle;
		if (auto It = _IndexByEntity.find(EntityID))
		{
			// Explicit component addition always overrides a template, even for existing components.
			// If existing component is not loaded, this function doesn't load it and returns nullptr.
			It->Value.State = EComponentState::Explicit;
			Handle = It->Value.ComponentHandle;
		}
		else
		{
			Handle = _Data.Allocate({ {}, EntityID });
			if (Handle) _IndexByEntity.emplace(EntityID,
				CIndexRecord{ NO_BASE_DATA, {}, 0, Handle, EComponentState::Explicit, EComponentState::NoBase });
		}

		return Handle ? &_Data.GetValueUnsafe(Handle)->first : nullptr;
	}
	//---------------------------------------------------------------------

	// TODO: describe as a static interface part
	bool Remove(HEntity EntityID)
	{
		// No record - nothing to remove
		auto It = _IndexByEntity.find(EntityID);
		if (!It) FAIL;

		// Unload removed component
		auto& IndexRecord = It->Value;
		if (IndexRecord.ComponentHandle)
		{
			_Data.Free(IndexRecord.ComponentHandle);
			IndexRecord.ComponentHandle = CInnerStorage::INVALID_HANDLE;
		}

		if (IndexRecord.BaseSate == EComponentState::NoBase)
		{
			// Component won't be restored from base on loading, so can delete all info
			_IndexByEntity.erase(It);
		}
		else
		{
			// To track diff properly we can't erase the whole record
			IndexRecord.State = EComponentState::Deleted;
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

	virtual bool LoadComponentFromParams(HEntity EntityID, const Data::CData& In, bool Replace) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		auto It = _IndexByEntity.find(EntityID);
		if (It && !Replace) return false;

		T* pComponent = It ? &_Data.GetValueUnsafe(It->Value.ComponentHandle)->first : Add(EntityID);
		if (!pComponent) return false;

		DEM::ParamsFormat::Deserialize(In, *pComponent);
		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		if (auto pComponent = Find(EntityID))
		{
			NOT_IMPLEMENTED;
			//if (IsComponentEqualToTemplate(EntityID, *pComponent)) return false;
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

		NOT_IMPLEMENTED;
		//if (IndexRecord.Deleted)
		{
			// Explicitly save as deleted
			Out = Data::CData();
			return true;
		}
		//else
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
					if constexpr (STORAGE_USE_DIFF_POOL<T>)
						DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(IndexRecord.DiffData, MAX_DIFF_SIZE)), Component);
					else
						DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(IndexRecord.DiffData.GetConstPtr(), IndexRecord.DiffData.GetSize())), Component);
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

		// Load explicitly deleted records. This prevents templated component instantiation.
		const auto DeletedCount = In.Read<U32>();
		for (U32 i = 0; i < DeletedCount; ++i)
		{
			const auto EntityIDRaw = In.Read<decltype(HEntity::Raw)>();
			_IndexByEntity.emplace(HEntity{ EntityIDRaw },
				CIndexRecord{ NO_BASE_DATA, {}, 0, CHandle(), EComponentState::Deleted, EComponentState::Deleted });
		}

		// We could create base components from data right here, but we will deserialize them on demand instead
		const auto IndexCount = In.Read<U32>();
		for (U32 i = 0; i < IndexCount; ++i)
		{
			const auto EntityIDRaw = In.Read<decltype(HEntity::Raw)>();
			const auto OffsetInBase = In.Read<U64>();
			const auto ComponentState = static_cast<EComponentState>(In.Read<U8>());
			_IndexByEntity.emplace(HEntity{ EntityIDRaw },
				CIndexRecord{ OffsetInBase, {}, 0, CHandle(), ComponentState, ComponentState });
		}

		// Skip binary data for now. Will be accessed through records' OffsetInBase
		auto ComponentDataSkipOffset = In.Read<U64>();
		In.GetStream().Seek(static_cast<I64>(ComponentDataSkipOffset), IO::ESeekOrigin::Seek_Begin);

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;

		// Instead of cleaning all up, we only erase all diff data and unload components modified by it

		std::vector<HEntity> RecordsToDelete;
		RecordsToDelete.reserve(_Data.size() / 4);
		_IndexByEntity.ForEach([this, &RecordsToDelete](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.BaseSate == EComponentState::NoBase)
			{
				// Created at runtime and must be deleted entirely
				RecordsToDelete.push_back(EntityID);
			}
			else
			{
				// If already in a base state, do nothing. Component is not invalidated.
				//???what if was not saved to DiffData/State yet, but the component has changed?
				if (!Record.DiffDataSize && Record.State == Record.BaseState) return; // continue

				// Erase difference from base
				ClearDiffBuffer(Record);
				Record.State = Record.BaseState;
			}

			// Unload invalidated component
			if (Record.ComponentHandle)
			{
				_Data.Free(Record.ComponentHandle);
				Record.ComponentHandle = CInnerStorage::INVALID_HANDLE; //???keep handle to save handle space? is index enough?
			}
		});

		// TODO: could load new diffs, removing recreated records from RecordsToDelete, and then delete!
		// Must then ensure that all diff data is updated in these records!
		for (auto EntityID : RecordsToDelete)
			_IndexByEntity.erase(EntityID);

		// Load deleted component list, mark corresponding records as deleted

		HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			if (auto It = _IndexByEntity.find(EntityID))
				It->Value.State = EComponentState::Deleted;
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		// Load diff data for added and modified components

		EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			auto It = _IndexByEntity.find(EntityID);
			if (!It) It = _IndexByEntity.emplace(EntityID, CIndexRecord()); // Runtime-created record without base data
			auto& IndexRecord = It->Value;

			IndexRecord.State = static_cast<EComponentState>(In.Read<U8>()); // Explicit or Templated

			In.Read(IndexRecord.DiffDataSize);

			if constexpr (STORAGE_USE_DIFF_POOL<T>)
			{
				if (!IndexRecord.DiffData) IndexRecord.DiffData = _DiffPool.Allocate();
				In.GetStream().Read(IndexRecord.DiffData, IndexRecord.DiffDataSize);
			}
			else
			{
				IndexRecord.DiffData.Resize(IndexRecord.DiffDataSize);
				In.GetStream().Read(IndexRecord.DiffData.GetPtr(), IndexRecord.DiffDataSize);
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

		struct CRecordData
		{
			HEntity         EntityID;
			size_t          BinaryDataIndex;
			EComponentState ComponentState;
		};

		std::vector<std::pair<Data::CBufferMalloc, U64>> BinaryData;
		std::vector<CRecordData> EntityToData;
		std::vector<HEntity> DeletedRecords;
		BinaryData.reserve(_IndexByEntity.size());
		EntityToData.reserve(_IndexByEntity.size());
		DeletedRecords.reserve(_IndexByEntity.size());

		// Choose some reasonable initial size for the buffer to minimize reallocations.
		// Max diff size is always more than max whole data size due to field IDs, so it is safe to choose it.
		IO::CMemStream IntermediateStream(std::min<decltype(MAX_DIFF_SIZE)>(MAX_DIFF_SIZE, 512));
		IO::CBinaryWriter Intermediate(IntermediateStream);

		_IndexByEntity.ForEach([&](HEntity EntityID, const CIndexRecord& Record)
		{
			Intermediate.GetStream().Seek(0, IO::Seek_Begin);

			if (Record.State == EComponentState::Explicit)
			{
				// Explicit component overrides the template, all its data is saved
				DEM::BinaryFormat::Serialize(Intermediate,
					Record.ComponentHandle ?
					_Data.GetValueUnsafe(Record.ComponentHandle)->first :
					LoadComponent(EntityID, Record));
			}
			else
			{
				// Templated and Deleted records save the difference against the template
				auto pEntity = _World.GetEntity(EntityID);
				if (!pEntity || !pEntity->TemplateID) return; // continue
					
				if (Record.State == EComponentState::Templated)
				{
					// Record is templated and no template is present. Save nothing, ignore possible diff.
					T TplComponent;
					if (!_World.GetTemplateComponent<T>(pEntity->TemplateID, TplComponent)) return; // continue

					// Save only the difference against the template, so changes in defaulted fields will be propagated
					// from templates to instances when template is changed but the world base file is not.
					const bool HasDiff = DEM::BinaryFormat::SerializeDiff(Intermediate,
						Record.ComponentHandle ?
						_Data.GetValueUnsafe(Record.ComponentHandle)->first :
						LoadComponent(EntityID, Record),
						TplComponent);

					// Component is equal to template, nothing to save
					if (!HasDiff) return; // continue
				}
				else // EComponentState::Deleted
				{
					// Write explicit deletion only if template component is present, otherwise there is nothing to delete
					if (_World.HasTemplateComponent<T>(pEntity->TemplateID))
						DeletedRecords.push_back(EntityID);
					return; // continue
				}
			}

			const auto SerializedSize = static_cast<UPTR>(Intermediate.GetStream().Tell());
			if (!SerializedSize) return; // continue

			const void* pComponentData = Intermediate.GetStream().Map();

			//???TODO: use hashes for faster comparison?
			for (size_t i = 0; i < BinaryData.size(); ++i)
				if (!BinaryData[i].first.Compare(pComponentData, SerializedSize))
				{
					EntityToData.push_back({ EntityID, i, Record.State });
					return;
				}

			Data::CBufferMalloc NewBuffer(SerializedSize);
			memcpy(NewBuffer.GetPtr(), pComponentData, SerializedSize);
			BinaryData.emplace_back(std::move(NewBuffer), 0);

			EntityToData.push_back({ EntityID, BinaryData.size() - 1, Record.State });
		});

		// Save explicitly deleted components
		Out.Write(static_cast<U32>(DeletedRecords.size()));
		for (const auto& EntityID : DeletedRecords)
			Out.Write(EntityID.Raw);

		// Build EntityID -> component data offset index table for template-overriding components and template diffs.
		// Base offset of component data is current offset plus size of indexing table, written just below.
		U64 ComponentDataOffset = Out.GetStream().Tell() +
			sizeof(U32) + // Index table size
			EntityToData.size() * (sizeof(decltype(HEntity::Raw)) + sizeof(U64) + sizeof(U8)) + // Index records
			sizeof(U64); // Resulting ComponentDataOffset for skip

		// Precalculate offsets per binary data record
		for (auto& [Buffer, Offset] : BinaryData)
		{
			Offset = ComponentDataOffset;
			ComponentDataOffset += Buffer.GetSize();
		}

		// Save index table
		Out.Write(static_cast<U32>(EntityToData.size()));
		for (const auto& Record : EntityToData)
		{
			Out.Write(Record.EntityID.Raw);
			Out.Write(BinaryData[Record.BinaryDataIndex].second);
			Out.Write(static_cast<U8>(Record.ComponentState));
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
			if (Record.State == EComponentState::Deleted)
				Out.Write(EntityID.Raw);
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		// Save diff data of added and modified components

		_IndexByEntity.ForEach([this, &Out](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.ComponentHandle) SaveComponent(EntityID, Record);
			Out.Write(static_cast<U8>(Record.State));
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
			// Already validated
			if (Record.ComponentHandle) return;

			// Deleted, no component required
			if (Record.State == EComponentState::Deleted) return;

			// Not in a level being validated
			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			// Templated component without a template, no component required, even if the diff is present
			if (Record.State == EComponentState::Templated && !_World.HasTemplateComponent<T>(pEntity->TemplateID)) return;

			Record.ComponentHandle = _Data.Allocate({ std::move(LoadComponent(EntityID, Record)), EntityID });
			n_assert_dbg(Record.ComponentHandle);
		});
	}
	//---------------------------------------------------------------------

	virtual void InvalidateComponents(CStrID LevelID) override
	{
		_IndexByEntity.ForEach([this, LevelID](HEntity EntityID, CIndexRecord& Record)
		{
			// Already unloaded
			if (!Record.ComponentHandle) return;

			// Not in a level being invalidated
			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			SaveComponent(EntityID, Record);

			_Data.Free(Record.ComponentHandle);
			Record.ComponentHandle = CInnerStorage::INVALID_HANDLE; //???TODO: keep handle to save handle space? is index enough?
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

	enum
	{
		PresentInCurrState = 0x01,
		PresentInBaseState = 0x02,
		OverridesTemplate  = 0x04
	};

	CEntityMap<U8>    _IndexByEntity;  // EntityID to flags
	T                 _SharedInstance; // It is enough to have one instance of component without data

public:

	CEmptyComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
		, _IndexByEntity(InitialCapacity)
	{
	}

	DEM_FORCE_INLINE T* Add(HEntity EntityID)
	{
		// Explicitly added components always override templates.
		// Components from templates are created through LoadComponentFromParams.
		if (auto It = _IndexByEntity.find(EntityID))
			It->Value |= (PresentInCurrState | OverridesTemplate);
		else
			_IndexByEntity.emplace(EntityID, PresentInCurrState | OverridesTemplate);
		return &_SharedInstance;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE bool Remove(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;

		//???template? remove only if no binding to template?
		//???or on template change must explicitly process?
		if (It->Value == PresentInCurrState)
			_IndexByEntity.erase(It);
		else
			It->Value &= ~PresentInCurrState;

		return true;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		return (It && (It->Value & PresentInCurrState)) ? &_SharedInstance : nullptr;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		return (It && (It->Value & PresentInCurrState)) ? &_SharedInstance : nullptr;
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

	virtual bool LoadComponentFromParams(HEntity EntityID, const Data::CData& In, bool /*Replace*/) override
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
		if (!Find(EntityID)) return false;
		Out = true;
		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;

		const bool HasBase = (It->Value & PresentInBaseState);
		const bool HasCurr = (It->Value & PresentInCurrState);
		if (HasBase == HasCurr) return false;

		if (HasBase) Out = Data::CData();
		else Out = true;

		return true;
	}
	//---------------------------------------------------------------------

	// All non-explicit components will be registered from template instantiation
	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		_IndexByEntity.clear();

		// Read explicitly deleted list
		HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			n_assert_dbg(!_IndexByEntity.find(EntityID));
			_IndexByEntity.emplace(EntityID, OverridesTemplate);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		// Read explicitly added list
		EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			n_assert_dbg(!_IndexByEntity.find(EntityID));
			_IndexByEntity.emplace(EntityID, OverridesTemplate | PresentInBaseState);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		// TODO: LoadDiff / FinalizeLoading must actualize PresentInCurrState

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		// First set the current state from the base one
		_IndexByEntity.ForEach([](HEntity EntityID, U8& Flags)
		{
			if (Flags & PresentInBaseState)
				Flags |= PresentInCurrState;
			else
				Flags &= ~PresentInCurrState;
		});

		// Then apply deletions from the diff
		HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			Remove(EntityID);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		// And finally apply additions
		EntityID = { In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			Add(EntityID);
			EntityID = { In.Read<decltype(HEntity::Raw)>() };
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		// Save only records that must override templates. Deleted, than added.

		_IndexByEntity.ForEach([&Out](HEntity EntityID, U8& Flags)
		{
			if ((Flags & OverridesTemplate) && !(Flags | PresentInCurrState))
				Out << EntityID.Raw;
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		_IndexByEntity.ForEach([&Out](HEntity EntityID, U8& Flags)
		{
			if ((Flags & OverridesTemplate) && (Flags | PresentInCurrState))
				Out << EntityID.Raw;
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveDiff(IO::CBinaryWriter& Out) override
	{
		// Save the list of deleted components

		_IndexByEntity.ForEach([&Out](HEntity EntityID, U8& Flags)
		{
			if ((Flags & PresentInBaseState) && !(Flags & PresentInCurrState))
				Out << EntityID.Raw;
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		// Save the list of added components

		_IndexByEntity.ForEach([&Out](HEntity EntityID, U8& Flags)
		{
			if (!(Flags & PresentInBaseState) && (Flags & PresentInCurrState))
			Out << EntityID.Raw;
		});

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
