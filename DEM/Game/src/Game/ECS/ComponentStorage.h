#pragma once
#include <Game/ECS/Entity.h>
#include <Core/RTTIBaseClass.h>

// Stores components of specified type. Each component type can have only one storage type.
// Interface is provided for the game world to handle all storages transparently. In places
// where the component type is known it is preferable to use static storage type.
// TComponentTraits determine what type of storage which component type uses.
// Effective and compact handle array storage is used by default.

namespace DEM::Game
{
typedef std::unique_ptr<class IComponentStorage> PComponentStorage;

class IComponentStorage : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL;

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
	//???can add automatic or leave some functions pure virtual and make user
	// inherit IComponentStorage / CHandleArrayComponentStorage for each component?
	//RTTI_CLASS_DECL;
	//#define ENABLE_TYPENAME(A) template<> struct TypeName<A> { constexpr char* Get() { return #A; }};
//#define RTTI_CLASS_IMPL(Class, ParentClass) \
	//::Core::CRTTI Class::RTTI(TypeName<Class>::Get(), 0, nullptr, &ParentClass::RTTI, 0);

public:

	using CInnerStorage = Data::CHandleArray<T, H, IndexBits, ResetOnOverflow>;

	CInnerStorage                   Data;
	std::unordered_map<HEntity, T*> IndexByEntity;

	CHandleArrayComponentStorage(UPTR InitialCapacity) : Data(InitialCapacity) {}

	virtual bool LoadComponent() { return false; }
	virtual bool SaveComponent() const { return false; }
};

}
