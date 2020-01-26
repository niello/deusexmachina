#pragma once
#include <Data/HandleArray.h>
#include <Game/ECS/Entity.h>

// Stores components of specified type. Each component type can have only one storage type.
// Interface is provided for the game world to handle all storages transparently. In places
// where the component type is known it is preferable to use static storage type.
// TComponentTraits determine what type of storage which component type uses.
// Effective and compact handle array storage is used by default.

namespace DEM::Game
{

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

	CInnerStorage                   Data;
	std::unordered_map<HEntity, T*> IndexByEntity;

	virtual bool LoadComponent() {}
	virtual bool SaveComponent() const {}
};

template<class T>
struct TComponentTraits
{
	using TStorage = CHandleArrayComponentStorage<T>;
	using THandle = typename TStorage::CHandle;
};

}
