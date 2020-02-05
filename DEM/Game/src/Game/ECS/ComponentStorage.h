#pragma once
#include <Game/ECS/Entity.h>
#include <Game/ECS/EntityComponentMap.h>

// Stores components of specified type. Each component type can have only one storage type.
// Interface is provided for the game world to handle all storages transparently. In places
// where the component type is known it is preferable to use static storage type.
// TComponentTraits determine what type of storage which component type uses.
// Effective and compact handle array storage is used by default.

namespace DEM::Game
{
typedef std::unique_ptr<class IComponentStorage> PComponentStorage;

class IComponentStorage
{
public:

	virtual ~IComponentStorage() = default;

	virtual CStrID GetComponentName() const = 0;
	virtual bool   LoadComponent(HEntity EntityID /*, In*/) = 0;
	virtual bool   SaveComponent(HEntity EntityID /*, Out*/) const = 0;
};

//, typename = std::enable_if_t<std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>>
template<typename T, typename H = uint32_t, size_t IndexBits = 18, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage
{
public:

	using CInnerStorage = Data::CHandleArray<T, H, IndexBits, ResetOnOverflow>;
	using CHandle = typename CInnerStorage::CHandle;

protected:

	CInnerStorage                _Data;
	CEntityComponentMap<CHandle> _IndexByEntity;

public:

	CHandleArrayComponentStorage(UPTR InitialCapacity)
		: _Data(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
	{
		n_assert_dbg(InitialCapacity <= CInnerStorage::MAX_CAPACITY);
	}

	// TODO: describe as a static interface part
	T* Add(HEntity EntityID)
	{
		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		auto It = _IndexByEntity.find(EntityID);
		if (It != _IndexByEntity.cend()) return _Data.GetValueUnsafe(*It);

		auto Handle = _Data.Allocate();
		T* pComponent = _Data.GetValueUnsafe(Handle);
		if (!pComponent) return nullptr;

		pComponent->EntityID = EntityID;
		_IndexByEntity.emplace(EntityID, Handle);
		return pComponent;
	}

	// TODO: describe as a static interface part
	bool Remove(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (It == _IndexByEntity.cend()) FAIL;

		_Data.Free(*It);
		_IndexByEntity.erase(It);
		OK;
	}

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE T* Find(HEntity EntityID)
	{
		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		auto It = _IndexByEntity.find(EntityID);
		return (It == _IndexByEntity.cend()) ? nullptr : _Data.GetValueUnsafe(*It);
	}

	// TODO: remove by CHandle
	// TODO: find by CHandle

	//!!!could call T::Load/T::Save non-virtual methods here and avoid subclassing storages explicitly for every component!
	//can even have templated methods outside structs if it is cleaner for some reason, but probably not.
	//The one benefit of template (traits-like) external method is an empty default implementation Save/Load(){}.
	virtual bool LoadComponent(HEntity EntityID /*, In*/) { return false; }
	virtual bool SaveComponent(HEntity EntityID /*, Out*/) const { return false; }

	auto begin() { return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }
};

}
