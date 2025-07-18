#pragma once
#include <Game/ECS/EntityMap.h>
#include <Data/SparseArray.hpp>
#include <Data/SerializeToParams.h>
#include <Data/SerializeToBinary.h>
#include <Data/Buffer.h>
#include <Events/Signal.h>
#include <IO/Streams/MemStream.h>
#include <System/Allocators/PoolAllocator.h>

// Stores components of specified type. Each component type can have only one storage type.
// Interface is provided for the game world to handle all storages transparently. In places
// where the component type is known it is preferable to use static storage type.
// TComponentTraits determine what type of storage which component type uses.

namespace DEM::Game
{
class CGameWorld;
typedef std::unique_ptr<class IComponentStorage> PComponentStorage;

///////////////////////////////////////////////////////////////////////
// Component storage interface
///////////////////////////////////////////////////////////////////////

// NB: values are saved to file, don't change them
enum class EComponentState : U8
{
	NoBase = 0, // Component is created at runtime (used only for base state)
	Templated,  // Component is loaded from the template with optional per-instance diff
	Explicit,   // Component is explicitly added, template will be ignored
	Deleted     // Component is explicitly deleted, template will be ignored
};

class IComponentStorage
{
protected:

	const CGameWorld& _World;

public:

	IComponentStorage(const CGameWorld& World) : _World(World) {}
	virtual ~IComponentStorage() = default;

	virtual bool CloneComponent(HEntity SrcEntityID, HEntity DestEntityID) = 0;
	virtual bool RemoveComponent(HEntity EntityID) = 0;
	virtual void InstantiateTemplate(HEntity EntityID, bool BaseState, bool Validate) = 0;

	virtual bool AddFromParams(HEntity EntityID, const Data::CData& In) = 0;
	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const = 0;
	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const = 0;
	virtual bool LoadBase(IO::CBinaryReader& In) = 0;
	virtual bool LoadDiff(IO::CBinaryReader& In) = 0;
	virtual bool SaveAll(IO::CBinaryWriter& Out) const = 0;
	virtual bool SaveDiff(IO::CBinaryWriter& Out) = 0;
	virtual void ClearAll() = 0;
	virtual void ClearDiff() = 0;

	virtual void ValidateComponents(CStrID LevelID) = 0;
	virtual void InvalidateComponents(CStrID LevelID) = 0;
};

///////////////////////////////////////////////////////////////////////
// Default storage with component vector and a special map for entity -> component indexing
///////////////////////////////////////////////////////////////////////

// Conditional pool member for binary diff data (Base/Current)
template<typename T> constexpr bool STORAGE_USE_DIFF_POOL = (DEM::BinaryFormat::GetMaxDiffSize<T>() <= 512);
template<typename T> struct CStoragePool
{
	CPoolAllocator<DEM::BinaryFormat::GetMaxDiffSize<T>()> _DiffPool;

	~CStoragePool() { _DiffPool.Clear(); }
};
struct CStorageNoPool {};

// Conditional signals mixin
template<typename T> struct CStorageSignals
{
	Events::CSignal<void(HEntity, T&)> OnAdd;
	Events::CSignal<void(HEntity, T&)> OnRemove;
	Events::CSignal<void(HEntity, T&)> OnDestroy;
	//???OnModify / OnGetMutable?
};
struct CStorageNoSignals {};

template<typename T, bool Signals = false>
class CSparseComponentStorage : public IComponentStorage,
	std::conditional_t<STORAGE_USE_DIFF_POOL<T>, CStoragePool<T>, CStorageNoPool>,
	public std::conditional_t<Signals, CStorageSignals<T>, CStorageNoSignals>
{
protected:

	using TInnerStorage = Data::CSparseArray<std::pair<T, HEntity>, U32>;

	constexpr static inline auto INVALID_INDEX = TInnerStorage::INVALID_INDEX;
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
		U32             Index = INVALID_INDEX;               // Index in _Data. Invalid if component is not loaded.
		EComponentState State = EComponentState::Templated;
		EComponentState BaseState = EComponentState::NoBase; // For on-demand base data loading, read-only
		bool            DiffDirty = false;                   // For save optimization
	};

	TInnerStorage            _Data;
	CEntityMap<CIndexRecord> _IndexByEntity;

	void ValidateComponent(HEntity EntityID, CIndexRecord& Record)
	{
		n_assert_dbg(Record.Index == INVALID_INDEX && !Record.DiffDirty);
		Record.Index = _Data.emplace(LoadComponent(EntityID, Record), EntityID);
		n_assert_dbg(Record.Index != INVALID_INDEX);
	}
	//---------------------------------------------------------------------

	void ClearComponent(CIndexRecord& Record)
	{
		if (Record.Index == INVALID_INDEX) return;

		if constexpr (Signals) OnDestroy(_Data[Record.Index].second, _Data[Record.Index].first);

		_Data.erase(Record.Index);
		Record.Index = INVALID_INDEX;
		Record.DiffDirty = false;
	}
	//---------------------------------------------------------------------

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
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else if (Record.BaseState == EComponentState::Explicit)
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
			if (auto pTplData = _World.GetTemplateComponentData<T>(EntityID))
			{
				DEM::ParamsFormat::Deserialize(*pTplData, Component);
				if (auto pBaseStream = _World.GetBaseStream(Record.BaseDataOffset))
					DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(*pBaseStream), Component);
				return true;
			}
		}
		else if (Record.State == EComponentState::Templated)
		{
			// Runtime-created template components base on templates
			if (auto pTplData = _World.GetTemplateComponentData<T>(EntityID))
			{
				DEM::ParamsFormat::Deserialize(*pTplData, Component);
				return true;
			}
		}

		return false;
	}
	//---------------------------------------------------------------------

	// Restores a component from base data and binary diff
	T LoadComponent(HEntity EntityID, const CIndexRecord& Record) const
	{
		T Component;
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			LoadBaseComponent(EntityID, Record, Component);

			// If diff data is available, apply it on top of base data
			if (Record.DiffDataSize)
			{
				if constexpr (STORAGE_USE_DIFF_POOL<T>)
					DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData, Record.DiffDataSize)), Component);
				else
					DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData.GetConstPtr(), Record.DiffDataSize)), Component);
			}
		}
		return Component;
	}
	//---------------------------------------------------------------------

	// Converts a component to binary diff against the base
	void SaveComponent(CIndexRecord& Record)
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return;
		else
		{
			if (!Record.DiffDirty) return;

			n_assert2_dbg(Record.Index != INVALID_INDEX, "CSparseComponentStorage::SaveComponent() > call only for loaded components");

			const auto& [Component, EntityID] = _Data[Record.Index];

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
				// FIXME: need more clever logic? vector-like allocation strategy inside a CBufferMalloc, or std::vector-based buffer?
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
				// When HasDiff is false, the component is new but equal to the default one, and we must
				// save empty diff, which is not equal to nothing. It is already written in SerializeDiff.
				if constexpr (!STORAGE_USE_DIFF_POOL<T>)
				{
					// Truncate if too many unused bytes left
					// FIXME: need more clever logic?
					if (Record.DiffData.GetSize() > Record.DiffDataSize + 400)
						Record.DiffData.Resize(Record.DiffDataSize);
				}
			}

			Record.DiffDirty = false;
		}
	}
	//---------------------------------------------------------------------

	// NB: can't embed into lambdas, both branches of if constexpr are compiled for some reason
	static inline void WriteComponentDiff(IO::CBinaryWriter& Out, HEntity EntityID, const CIndexRecord& Record)
	{
		if (Record.DiffDataSize)
		{
			Out.Write(EntityID.Raw);
			Out.Write(static_cast<U8>(Record.State));
			Out.Write(Record.DiffDataSize);
			if constexpr (STORAGE_USE_DIFF_POOL<T>)
				Out.GetStream().Write(Record.DiffData, Record.DiffDataSize);
			else
				Out.GetStream().Write(Record.DiffData.GetConstPtr(), Record.DiffDataSize);
		}
	}
	//---------------------------------------------------------------------

	// Writes full component data
	template<typename TFormat, typename TOutput>
	bool WriteComponent(TOutput& Out, HEntity EntityID, const CIndexRecord& Record) const
	{
		if (Record.State == EComponentState::Explicit)
		{
			// Explicit component overrides the template, all its data is saved
			if (Record.Index != INVALID_INDEX)
				TFormat::Serialize(Out, _Data[Record.Index].first);
			else
				TFormat::Serialize(Out, LoadComponent(EntityID, Record));
			return true;
		}
		else
		{
			// Templated and Deleted records have meaning only when template exists.
			// If no template, write nothing (nor diff neither explicit deletion).
			auto pTplData = _World.GetTemplateComponentData<T>(EntityID);
			if (!pTplData) return false;

			if (Record.State == EComponentState::Templated)
			{
				// If record is templated and no template is present, diff has no meaning because there is nothing
				// to compare with. So diff is ignored, and record will be purely templated (not saved at all).
				T TplComponent;
				DEM::ParamsFormat::Deserialize(*pTplData, TplComponent);

				// Save only the difference against the template, so changes in defaulted fields will be propagated
				// from templates to instances when template is changed, even if the world base file is not rebuilt.
				// If component is equal to template, save nothing. It will be created on template instantiation.
				return (Record.Index != INVALID_INDEX) ?
					TFormat::SerializeDiff(Out, _Data[Record.Index].first, TplComponent) :
					TFormat::SerializeDiff(Out, LoadComponent(EntityID, Record), TplComponent);
			}
			else // always EComponentState::Deleted, because State can never be EComponentState::NoBase
			{
				// Explicit deletion will be written by the calling code
				n_assert_dbg(Record.State != EComponentState::NoBase);
				return true;
			}
		}
	}
	//---------------------------------------------------------------------

	// When non-const iterator is requested, must mark all instantiated components as dirty.
	// This must be faster than looking up each one separately from inside the iterator on access.
	void MarkAllDirty()
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			_IndexByEntity.ForEach([](HEntity, CIndexRecord& Record)
			{
				if (Record.Index != INVALID_INDEX) Record.DiffDirty = true;
			});
		}
	}
	//---------------------------------------------------------------------

public:

	CSparseComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
		, _Data(std::min<size_t>(InitialCapacity, TInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, TInnerStorage::MAX_CAPACITY))
	{
		n_assert_dbg(InitialCapacity <= TInnerStorage::MAX_CAPACITY);
	}

	// Explicitly adds a component. If templated one is present, detaches it from the template.
	T* Add(HEntity EntityID)
	{
		U32 Index = INVALID_INDEX;
		if (auto It = _IndexByEntity.find(EntityID))
		{
			// Always set the current state of existing component to requested value.
			// If existing component is not loaded, this function doesn't load it and returns nullptr.
			It->Value.State = EComponentState::Explicit;
			It->Value.DiffDirty = true;
			Index = It->Value.Index;
		}
		else
		{
			Index = _Data.emplace(T{}, EntityID);
			if (Index != INVALID_INDEX)
				_IndexByEntity.emplace(EntityID, CIndexRecord{ NO_BASE_DATA, {}, 0, Index, EComponentState::Explicit, EComponentState::NoBase, true });
		}

		if (Index == INVALID_INDEX) return nullptr;

		if constexpr (Signals) OnAdd(EntityID, _Data[Index].first);

		return &_Data[Index].first;
	}
	//---------------------------------------------------------------------

	virtual bool CloneComponent(HEntity SrcEntityID, HEntity DestEntityID)
	{
		auto It = _IndexByEntity.find(SrcEntityID);
		if (!It) return false;

		T* pDst = Add(DestEntityID);
		if (!pDst) return false;

		if constexpr (std::is_copy_assignable_v<T>)
		{
			*pDst = _Data[It->Value.Index].first;
			return true;
		}
		else
		{
			// Slower fallback through serialization
			if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return true;
			else
			{
				// Choose some reasonable initial size for the buffer to minimize reallocations.
				// Max diff size is always more than max whole data size due to field IDs, so it is safe to choose it.
				IO::CMemStream Stream(std::min<decltype(MAX_DIFF_SIZE)>(MAX_DIFF_SIZE, 512));
				if (WriteComponent<DEM::BinaryFormat>(IO::CBinaryWriter(Stream), SrcEntityID, It->Value))
				{
					DEM::BinaryFormat::Deserialize(IO::CBinaryReader(Stream), *pDst);
					return true;
				}

				// We failed, revert changes
				// TODO: may do serialization trial before adding dest entity but code will be less clean
				RemoveComponent(DestEntityID);
				return false;
			}
		}
	}
	//---------------------------------------------------------------------

	// Explicitly deletes a component, overrides a template if it exists
	virtual bool RemoveComponent(HEntity EntityID) override
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) FAIL;

		auto& IndexRecord = It->Value;

		if constexpr (Signals) OnRemove(EntityID, _Data[IndexRecord.Index].first);

		ClearComponent(IndexRecord);
		ClearDiffBuffer(IndexRecord);

		// If record has no template, it can be erased entirely. If component is later added to the template,
		// it will be instantiated. Can't explicitly delete templated component that is not present.
		if (IndexRecord.BaseState == EComponentState::NoBase && !_World.GetTemplateComponentData<T>(EntityID))
			_IndexByEntity.erase(It);
		else
			IndexRecord.State = EComponentState::Deleted;

		OK;
	}
	//---------------------------------------------------------------------

	// Restores existing record to templated state or erases it if no template exists.
	// NB: for full state only (editor), not supported in diffs (savegames). So BaseState is ignored.
	bool RevertToTemplate(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);

		//???if has tpl diff, allow erasing or always keep?

		if (_World.GetTemplateComponentData<T>(EntityID))
		{
			// Has templated component, setup a record for it
			if (It)
			{
				It->Value.State = EComponentState::Templated;
				It->Value.DiffDirty = true;
			}
			else
			{
				_IndexByEntity.emplace(EntityID,
					CIndexRecord{ NO_BASE_DATA, {}, 0, INVALID_INDEX, EComponentState::Templated, EComponentState::NoBase });
			}
		}
		else if (It)
		{
			// No templated component, erase record
			auto& IndexRecord = It->Value;
			ClearComponent(IndexRecord);
			ClearDiffBuffer(IndexRecord);
			_IndexByEntity.erase(It);
		}

		return true;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || It->Value.Index == INVALID_INDEX) return nullptr;
		It->Value.DiffDirty = true;
		return &_Data[It->Value.Index].first;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || It->Value.Index == INVALID_INDEX) return nullptr;
		return &_Data[It->Value.Index].first;
	}
	//---------------------------------------------------------------------

	virtual void InstantiateTemplate(HEntity EntityID, bool BaseState, bool Validate) override
	{
		// Don't replace existing records, only add purely templated ones, that aren't loaded in LoadBase
		if (_IndexByEntity.find(EntityID)) return;

		auto It = _IndexByEntity.emplace(EntityID,
			CIndexRecord{ NO_BASE_DATA, {}, 0, INVALID_INDEX, EComponentState::Templated, BaseState ? EComponentState::Templated : EComponentState::NoBase });

		// Gets here only if template component exists, no additional checks required
		if (Validate) ValidateComponent(EntityID, It->Value);
	}
	//---------------------------------------------------------------------

	virtual bool AddFromParams(HEntity EntityID, const Data::CData& In) override
	{
		if (!In.IsA<Data::PParams>()) return false;

		// Distinguish templated components from explicit
		const bool Templated = In.GetValue<Data::PParams>()->Get(CStrID("__UseTpl"), false);
		const auto State = Templated ? EComponentState::Templated : EComponentState::Explicit;

		// Add a record. Will replace existing one.
		auto It = _IndexByEntity.find(EntityID);
		if (It)
		{
			It->Value.State = State;
		}
		else
		{
			It = _IndexByEntity.emplace(EntityID,
				CIndexRecord{ NO_BASE_DATA, {}, 0, INVALID_INDEX, State, EComponentState::NoBase });
		}

		T Component;

		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			// Params format isn't supported as an intermediate storage, so we must create a component immediately.
			// NB: Component must be default-created for LoadBaseComponent to work correctly.
			LoadBaseComponent(EntityID, It->Value, Component);
			DEM::ParamsFormat::DeserializeDiff(In, Component);
		}

		if (It->Value.Index != INVALID_INDEX)
			_Data[It->Value.Index].first = std::move(Component);
		else
			It->Value.Index = _Data.emplace(std::move(Component), EntityID);

		It->Value.DiffDirty = true;

		if constexpr (Signals) OnAdd(EntityID, _Data[It->Value.Index].first);

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		//???!!!FIXME: make separate almost empty storage specialization for not registered components?
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
			auto It = _IndexByEntity.find(EntityID);
			if (!It) return false;
			const auto& Record = It->Value;

			// If there is nothing to write, return immediately
			if (!WriteComponent<DEM::ParamsFormat>(Out, EntityID, Record)) return false;

			// Perform format-specific part of writing
			if (Record.State == EComponentState::Templated)
				Out.GetValue<Data::PParams>()->Set(CStrID("__UseTpl"), true);
			else if (Record.State == EComponentState::Deleted)
				Out.Clear();

			return true;
		}
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
			auto It = _IndexByEntity.find(EntityID);
			if (!It) return false;
			const auto& Record = It->Value;

			if (Record.State == EComponentState::Deleted)
			{
				if (Record.BaseState == Record.State) return false;

				// Explicitly save as deleted
				Out.Clear();
				return true;
			}
			else
			{
				// Converting explicit components back to templated is not supported in diffs
				n_assert_dbg(Record.State != EComponentState::Templated || Record.BaseState == EComponentState::NoBase || Record.BaseState == Record.State);

				T BaseComponent;
				LoadBaseComponent(EntityID, Record, BaseComponent);

				bool HasDiff;
				if (Record.Index != INVALID_INDEX)
				{
					const T& Component = _Data[Record.Index].first;
					HasDiff = DEM::ParamsFormat::SerializeDiff(Out, Component, BaseComponent);
				}
				else
				{
					// TODO: how to handle non-copyable? Write CMetadata<T>::Move or load another copy of BaseComponent?
					// TODO: need CMetadata<T>::IsCopyable, depends on registered members!
					T Component;
					DEM::Meta::CMetadata<T>::Copy(BaseComponent, Component);

					if (Record.DiffDataSize)
					{
						if constexpr (STORAGE_USE_DIFF_POOL<T>)
							DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData, Record.DiffDataSize)), Component);
						else
							DEM::BinaryFormat::DeserializeDiff(IO::CBinaryReader(IO::CMemStream(Record.DiffData.GetConstPtr(), Record.DiffDataSize)), Component);
					}

					HasDiff = DEM::ParamsFormat::SerializeDiff(Out, Component, BaseComponent);
				}

				if (HasDiff && Record.State == EComponentState::Templated)
					Out.GetValue<Data::PParams>()->Set(CStrID("__UseTpl"), true);

				return HasDiff;
			}
		}
	}
	//---------------------------------------------------------------------

	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
		// Erase all existing data, both base and diff
			ClearAll();

			// Load explicitly deleted records. This prevents templated component instantiation.
			const auto DeletedCount = In.Read<U32>();
			for (U32 i = 0; i < DeletedCount; ++i)
			{
				const auto EntityIDRaw = In.Read<decltype(HEntity::Raw)>();
				_IndexByEntity.emplace(HEntity{ EntityIDRaw },
					CIndexRecord{ NO_BASE_DATA, {}, 0, INVALID_INDEX, EComponentState::Deleted, EComponentState::Deleted });
			}

			// We could create base components from data right here, but we will deserialize them on demand instead.
			// NB: purely templated components are not saved in a base, they are created on template instantiation.
			const auto IndexCount = In.Read<U32>();
			for (U32 i = 0; i < IndexCount; ++i)
			{
				const auto EntityIDRaw = In.Read<decltype(HEntity::Raw)>();
				const auto OffsetInBase = In.Read<U64>();
				const auto ComponentState = static_cast<EComponentState>(In.Read<U8>());
				_IndexByEntity.emplace(HEntity{ EntityIDRaw },
					CIndexRecord{ OffsetInBase, {}, 0, INVALID_INDEX, ComponentState, ComponentState });
			}

			//???OnAdd? or when instantiated?

			// Skip binary data for now. Will be accessed on demand through records' OffsetInBase.
			auto ComponentDataSkipOffset = In.Read<U64>();
			In.GetStream().Seek(static_cast<I64>(ComponentDataSkipOffset), IO::ESeekOrigin::Seek_Begin);

			return true;
		}
	}
	//---------------------------------------------------------------------

	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
		// Instead of cleaning all up, we only erase all diff data and unload affected components
			ClearDiff();

			// Remove components listed as deleted
			HEntity EntityID{ In.Read<decltype(HEntity::Raw)>() };
			while (EntityID)
			{
				RemoveComponent(EntityID);
				EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
			}

			// Load diff data for added and modified components
			EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
			while (EntityID)
			{
				auto It = _IndexByEntity.find(EntityID);
				if (It)
				{
					// Component could be kept loaded by ClearDiff, but now it becomes invalid and must be unloaded
					ClearComponent(It->Value);
				}
				else
				{
					// NB: State, DiffData and DiffDataSize will be overwritten below
					It = _IndexByEntity.emplace(EntityID,
						CIndexRecord{ NO_BASE_DATA, {}, 0, INVALID_INDEX, EComponentState::Templated, EComponentState::NoBase });
				}
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

				EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
			}

			return true;
		}
	}
	//---------------------------------------------------------------------

	// This method is primarily suited for saving levels in the editor, not for ingame use. Use SaveDiff to save game.
	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
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

				// If there is nothing to write, skip entity
				if (!WriteComponent<DEM::BinaryFormat>(Intermediate, EntityID, Record)) return; // continue

				// Perform format-specific part of writing
				if (Record.State == EComponentState::Deleted)
				{
					DeletedRecords.push_back(EntityID);
					return; // continue
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
				std::memcpy(NewBuffer.GetPtr(), pComponentData, SerializedSize);
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
				static_cast<U64>(EntityToData.size()) * (sizeof(decltype(HEntity::Raw)) + sizeof(U64) + sizeof(U8)) + // Index records
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
	}
	//---------------------------------------------------------------------

	virtual bool SaveDiff(IO::CBinaryWriter& Out) override
	{
		if constexpr (!DEM::Meta::CMetadata<T>::IsRegistered) return false;
		else
		{
		// Save the list of deleted components

			_IndexByEntity.ForEach([this, &Out](HEntity EntityID, CIndexRecord& Record)
			{
				if (Record.BaseState != Record.State && Record.State == EComponentState::Deleted)
					Out.Write(EntityID.Raw);
			});

			Out << CEntityStorage::INVALID_HANDLE_VALUE;

			// Save diff data of added and modified components

			_IndexByEntity.ForEach([this, &Out](HEntity EntityID, CIndexRecord& Record)
			{
				// Converting explicit components back to templated is not supported in diffs
				n_assert_dbg(Record.State != EComponentState::Templated || Record.BaseState == EComponentState::NoBase || Record.BaseState == Record.State);

				// State can be equal to BaseState, but some fields may change
				if (Record.State != EComponentState::Deleted)
				{
					if (Record.Index != INVALID_INDEX) SaveComponent(Record);
					WriteComponentDiff(Out, EntityID, Record);
				}
			});

			Out << CEntityStorage::INVALID_HANDLE_VALUE;

			return true;
		}
	}
	//---------------------------------------------------------------------

	virtual void ClearAll() override
	{
		if (_Data.empty()) return;

		for (auto& IndexRecord : _IndexByEntity)
		{
			if constexpr (Signals)
				if (IndexRecord.Index != INVALID_INDEX)
					OnDestroy(_Data[IndexRecord.Index].second, _Data[IndexRecord.Index].first);

			ClearDiffBuffer(IndexRecord);
		}
		_IndexByEntity.clear();
		_Data.clear();
	}
	//---------------------------------------------------------------------

	virtual void ClearDiff() override
	{
		std::vector<HEntity> RecordsToDelete;
		RecordsToDelete.reserve(_Data.size() / 4);
		_IndexByEntity.ForEach([this, &RecordsToDelete](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.BaseState == EComponentState::NoBase)
			{
				// Created at runtime and must be deleted entirely
				RecordsToDelete.push_back(EntityID);
			}
			else
			{
				// If base and current states are the same, component may be unchanged
				if (Record.State == Record.BaseState)
				{
					// Actualize diff data. Component may be changed but not saved yet.
					if (Record.Index != INVALID_INDEX) SaveComponent(Record);

					// If component is not changed, skip invalidation
					if (!Record.DiffDataSize) return; // continue
				}

				// Erase difference from base
				Record.State = Record.BaseState;
			}

			// Unload invalidated component
			ClearComponent(Record);
			ClearDiffBuffer(Record);
		});

		// TODO: could use erase/remove_if instead of copying IDs to the array, if CEntityMap will support it.
		// TODO: in LoadDiff could load new diffs, removing recreated records from RecordsToDelete, and then delete!
		// Must then ensure that all diff data is updated in these records!
		for (auto EntityID : RecordsToDelete)
			_IndexByEntity.erase(EntityID);
	}
	//---------------------------------------------------------------------

	virtual void ValidateComponents(CStrID LevelID) override
	{
		_IndexByEntity.ForEach([this, LevelID](HEntity EntityID, CIndexRecord& Record)
		{
			// Already validated
			if (Record.Index != INVALID_INDEX) return;

			// Deleted, no component required
			if (Record.State == EComponentState::Deleted) return;

			// Not in a level being validated
			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			// Templated component without a template, no component required, even if the diff is present
			if (Record.State == EComponentState::Templated && !_World.GetTemplateComponentData<T>(pEntity->TemplateID)) return;

			ValidateComponent(EntityID, Record);
		});
	}
	//---------------------------------------------------------------------

	virtual void InvalidateComponents(CStrID LevelID) override
	{
		_IndexByEntity.ForEach([this, LevelID](HEntity EntityID, CIndexRecord& Record)
		{
			// Already unloaded
			if (Record.Index == INVALID_INDEX) return;

			// Not in a level being invalidated
			auto pEntity = _World.GetEntity(EntityID);
			if (!pEntity || pEntity->LevelID != LevelID) return;

			SaveComponent(Record);
			ClearComponent(Record);
		});
	}
	//---------------------------------------------------------------------

	auto begin() { MarkAllDirty(); return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }

	size_t GetComponentCount() const { return _Data.size(); }
};

///////////////////////////////////////////////////////////////////////
// Default storage for empty components (flags)
///////////////////////////////////////////////////////////////////////

// NB: this storage doesn't fire OnDestroy signals because components here have no data
template<typename T, bool Signals = false>
class CEmptyComponentStorage : public IComponentStorage,
	public std::conditional_t<Signals, CStorageSignals<T>, CStorageNoSignals>
{
protected:

	struct CIndexRecord
	{
		EComponentState State = EComponentState::Templated;
		EComponentState BaseState = EComponentState::NoBase;
	};

	CEntityMap<CIndexRecord> _IndexByEntity;  // EntityID to flags
	T                        _SharedInstance; // It is enough to have one instance of component without data

public:

	CEmptyComponentStorage(const CGameWorld& World, UPTR InitialCapacity)
		: IComponentStorage(World)
		, _IndexByEntity(InitialCapacity)
	{
	}

	// Explicitly adds a component. If templated one is present, detaches it from the template.
	DEM_FORCE_INLINE T* Add(HEntity EntityID)
	{
		if (auto It = _IndexByEntity.find(EntityID))
			It->Value.State = EComponentState::Explicit;
		else
			_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Explicit, EComponentState::NoBase });

		if constexpr (Signals) OnAdd(EntityID, _SharedInstance);

		return &_SharedInstance;
	}
	//---------------------------------------------------------------------

	virtual bool CloneComponent(HEntity SrcEntityID, HEntity DestEntityID)
	{
		return Find(SrcEntityID) && Add(DestEntityID);
	}
	//---------------------------------------------------------------------

	// Explicitly deletes a component, overrides a template if it exists
	DEM_FORCE_INLINE virtual bool RemoveComponent(HEntity EntityID) override
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;

		if constexpr (Signals) OnRemove(EntityID, _SharedInstance);

		// If record has no template, it can be erased entirely. If component is later added to the template,
		// it will be instantiated. Can't explicitly delete templated component that is not present.
		if (It->Value.BaseState == EComponentState::NoBase && !_World.GetTemplateComponentData<T>(EntityID))
			_IndexByEntity.erase(It);
		else
			It->Value.State = EComponentState::Deleted;

		return true;
	}
	//---------------------------------------------------------------------

	// Restores existing record to templated state or erases it if no template exists.
	// NB: for full state only (editor), not supported in diffs (savegames). So BaseState is ignored.
	DEM_FORCE_INLINE bool RevertToTemplate(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (_World.GetTemplateComponentData<T>(EntityID))
		{
			// Has templated component, setup a record for it
			if (It)
				It->Value.State = EComponentState::Templated;
			else
				_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Templated, EComponentState::NoBase });
		}
		else
		{
			// No templated component, erase record
			if (It) _IndexByEntity.erase(It);
		}

		return true;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		return (It && It->Value.State != EComponentState::Deleted) ? &_SharedInstance : nullptr;
	}
	//---------------------------------------------------------------------

	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		auto It = _IndexByEntity.find(EntityID);
		return (It && It->Value.State != EComponentState::Deleted) ? &_SharedInstance : nullptr;
	}
	//---------------------------------------------------------------------

	virtual void InstantiateTemplate(HEntity EntityID, bool BaseState, bool /*Validate*/) override
	{
		// Don't replace existing records, only add purely templated ones, that aren't loaded in LoadBase
		if (_IndexByEntity.find(EntityID)) return;
		_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Templated, BaseState ? EComponentState::Templated : EComponentState::NoBase });
	}
	//---------------------------------------------------------------------

	virtual bool AddFromParams(HEntity EntityID, const Data::CData& In) override
	{
		EComponentState State;
		if (In.IsVoid()) State = EComponentState::Deleted;
		else if (In.IsA<bool>()) State = EComponentState::Explicit;
		else return false;

		if (auto It = _IndexByEntity.find(EntityID))
			It->Value.State = State;
		else
			_IndexByEntity.emplace(EntityID, CIndexRecord{ State, EComponentState::NoBase });

		if constexpr (Signals) OnAdd(EntityID, _SharedInstance);

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentToParams(HEntity EntityID, Data::CData& Out) const override
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It) return false;
		switch (It->Value.State)
		{
			case EComponentState::Deleted: Out.Clear(); return true;
			case EComponentState::Explicit: Out = true; return true;
		}
		return false; // Templated records aren't saved, they are instantiated from templates
	}
	//---------------------------------------------------------------------

	virtual bool SaveComponentDiffToParams(HEntity EntityID, Data::CData& Out) const override
	{
		auto It = _IndexByEntity.find(EntityID);
		if (!It || It->Value.BaseState == It->Value.State) return false;
		switch (It->Value.State)
		{
			case EComponentState::Deleted: Out.Clear(); return true;
			case EComponentState::Explicit: Out = true; return true;
			case EComponentState::Templated:
			{
				// Templated records aren't saved, they are instantiated from templates.
				// Revert from explicit to templated component is not supoported in diffs.
				n_assert(It->Value.BaseState == EComponentState::NoBase);
				return false;
			}
		}
		return false;
	}
	//---------------------------------------------------------------------

	virtual bool LoadBase(IO::CBinaryReader& In) override
	{
		_IndexByEntity.clear();

		// Read explicitly deleted list
		HEntity EntityID{ In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			n_assert_dbg(!_IndexByEntity.find(EntityID));
			_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Deleted, EComponentState::Deleted });
			EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		}

		// Read explicitly added list
		EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			n_assert_dbg(!_IndexByEntity.find(EntityID));
			_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Explicit, EComponentState::Explicit });
			EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool LoadDiff(IO::CBinaryReader& In) override
	{
		ClearDiff();

		// Read explicitly deleted list
		HEntity EntityID{ In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			if (auto It = _IndexByEntity.find(EntityID))
				It->Value.State = EComponentState::Deleted;
			else
				_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Deleted, EComponentState::NoBase });
			EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		}

		// Read explicitly added list
		EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		while (EntityID)
		{
			if (auto It = _IndexByEntity.find(EntityID))
				It->Value.State = EComponentState::Explicit;
			else
				_IndexByEntity.emplace(EntityID, CIndexRecord{ EComponentState::Explicit, EComponentState::NoBase });
			EntityID = HEntity{ In.Read<decltype(HEntity::Raw)>() };
		}

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveAll(IO::CBinaryWriter& Out) const override
	{
		// Explicitly deleted
		_IndexByEntity.ForEach([&Out](HEntity EntityID, const CIndexRecord& Record)
		{
			if (Record.State == EComponentState::Deleted)
				Out << EntityID.Raw;
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		// Explicitly added
		_IndexByEntity.ForEach([&Out](HEntity EntityID, const CIndexRecord& Record)
		{
			if (Record.State == EComponentState::Explicit)
				Out << EntityID.Raw;
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}
	//---------------------------------------------------------------------

	virtual bool SaveDiff(IO::CBinaryWriter& Out) override
	{
		// Templated records aren't saved, they are instantiated from templates.
		// Revert from explicit to templated component is not supoported in diffs.

		_IndexByEntity.ForEach([&Out](HEntity EntityID, const CIndexRecord& Record)
		{
			if (Record.BaseState != Record.State && Record.State == EComponentState::Deleted)
				Out.Write(EntityID.Raw);
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		_IndexByEntity.ForEach([&Out](HEntity EntityID, const CIndexRecord& Record)
		{
			if (Record.BaseState != Record.State && Record.State == EComponentState::Explicit)
				Out.Write(EntityID.Raw);
		});

		Out << CEntityStorage::INVALID_HANDLE_VALUE;

		return true;
	}
	//---------------------------------------------------------------------

	virtual void ClearAll() override
	{
		_IndexByEntity.clear();
	}
	//---------------------------------------------------------------------

	virtual void ClearDiff() override
	{
		std::vector<HEntity> RecordsToDelete;
		RecordsToDelete.reserve(_IndexByEntity.size() / 4);
		_IndexByEntity.ForEach([this, &RecordsToDelete](HEntity EntityID, CIndexRecord& Record)
		{
			if (Record.BaseState == EComponentState::NoBase)
				RecordsToDelete.push_back(EntityID);
			else
				Record.State = Record.BaseState;
		});

		for (auto EntityID : RecordsToDelete)
			_IndexByEntity.erase(EntityID);
	}
	//---------------------------------------------------------------------

	virtual void ValidateComponents(CStrID LevelID) override {}
	virtual void InvalidateComponents(CStrID LevelID) override {}

	size_t GetComponentCount() const { return _IndexByEntity.size(); }
};

///////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////

META_DECLARE_BOOL_FLAG(Signals);

template<typename T>
struct TComponentTraits
{
	using TStorage = std::conditional_t<std::is_empty_v<T>,
		CEmptyComponentStorage<T, is_bool_flag_Signals_v<T>>,
		CSparseComponentStorage<T, is_bool_flag_Signals_v<T>>>;
};

template<typename T>
using TComponentStoragePtr = std::conditional_t<std::is_const_v<T>,
	const typename TComponentTraits<just_type_t<T>>::TStorage*,
	typename TComponentTraits<just_type_t<T>>::TStorage*>;

}
