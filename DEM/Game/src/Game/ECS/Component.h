#pragma once
#include <Game/ECS/ComponentStorage.h>

// A piece of domain-specific data associated with an entity. Subclass it
// to add your own meaningful components. Note that components are not
// intended to store logic, systems are the right place for that. Some
// per-component-type logic like saving/loading can be implemented in
// component storages. This allows not to store virtual function table
// pointer per component instance and leads to better locality of data.

namespace DEM::Game
{

struct CComponent
{
	HEntity EntityID;
};

template<class T>
struct TComponentTraits
{
	using TStorage = CHandleArrayComponentStorage<T>;
	using THandle = typename TStorage::CInnerStorage::CHandle;
};

template<typename T> using TComponentStoragePtr = typename TComponentTraits<just_type_t<T>>::TStorage*;

}
