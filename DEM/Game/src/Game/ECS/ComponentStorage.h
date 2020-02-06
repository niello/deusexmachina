#pragma once
#include <Game/ECS/Entity.h>
#include <Game/ECS/EntityComponentMap.h>
#include <Data/SerializeToParams.h>
#include <Data/SerializeToBinary.h>

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

	virtual bool   LoadComponentFromParams(HEntity EntityID, const Data::CData& In) = 0;
	virtual bool   SaveComponentToParams(HEntity EntityID, Data::CData& Out) const = 0;
};

template<typename T, typename H = uint32_t, size_t IndexBits = 18, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage
{
public:

	using CInnerStorage = Data::CHandleArray<T, H, IndexBits, ResetOnOverflow>;
	using CHandle = typename CInnerStorage::CHandle;

protected:

	CInnerStorage                _Data;
	CEntityComponentMap<CHandle> _IndexByEntity;
	CStrID                       _ComponentName;

public:

	CHandleArrayComponentStorage(UPTR InitialCapacity)
		: _Data(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, CInnerStorage::MAX_CAPACITY))
		, _ComponentName(DEM::Meta::CMetadata<T>::GetClassName())
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

	// TODO: describe as a static interface part
	DEM_FORCE_INLINE const T* Find(HEntity EntityID) const
	{
		// NB: GetValueUnsafe is used because _IndexByEntity is guaranteed to be consistent with _Data
		auto It = _IndexByEntity.find(EntityID);
		return (It == _IndexByEntity.cend()) ? nullptr : _Data.GetValueUnsafe(*It);
	}

	// TODO: remove by CHandle
	// TODO: find by CHandle

	virtual CStrID GetComponentName() const override { return _ComponentName; }

	virtual bool LoadComponentFromParams(HEntity EntityID, const Data::CData& In) override
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
			if (auto pComponent = Add(EntityID))
			{
				DEM::ParamsFormat::Deserialize(In, *pComponent);
				return true;
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

	auto begin() { return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }
};

}
