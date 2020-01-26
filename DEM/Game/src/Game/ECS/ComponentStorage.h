#pragma once
#include <Game/ECS/Entity.h>

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

	// add, remove (performs indexing inside if required, allocates/frees objects etc)
	// remove by own handle and by HEntity

	virtual bool LoadComponent() = 0;
	virtual bool SaveComponent() const = 0;
};

template<typename T, typename H = uint32_t, size_t IndexBits = 17, bool ResetOnOverflow = true>
class CHandleArrayComponentStorage : public IComponentStorage
{
public:

	using CInnerStorage = Data::CHandleArray<T, H, IndexBits, ResetOnOverflow>;

protected:

	CInnerStorage                   _Data;
	std::unordered_map<HEntity, T*> _IndexByEntity;

public:

	CHandleArrayComponentStorage(UPTR InitialCapacity) : _Data(InitialCapacity) {}

	T* Add(HEntity EntityID)
	{
		auto It = _IndexByEntity.find(EntityID);
		if (It != _IndexByEntity.cend()) return It->second;

		auto Handle = _Data.Allocate();
		T* pComponent = _Data.GetValue(Handle);
		if (!pComponent) return nullptr;

		pComponent->EntityID = EntityID;
		_IndexByEntity.emplace(EntityID, pComponent);
		return EntityID;
	}

	//!!!could call T::Load/T::Save non-virtual methods here and avoid subclassing storages explicitly for every component!
	//can even have templated methods outside structs if it is cleaner for some reason, but probably not.
	//The one benefit of template (traits-like) external method is an empty default implementation.
	virtual bool LoadComponent() { return false; }
	virtual bool SaveComponent() const { return false; }
};

}
