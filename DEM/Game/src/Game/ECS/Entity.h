#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/HandleArray.h>

// Any world object is an entity, and entity itself is not anything in particular.
// All "meat" is stored inside components. In a pure ECS an entity must be no more
// than a plain identifier. Our entity, in contrary, has some data associated.
// You may think of it as of another component, which is mandatory for every entity.
// Storing this mandatory data right in the entity saves us one unnecessary component
// lookup, which otherwise would happen for every entity in every system every frame.
// CEntity must not be exposed to user code, except Systems. Use HEntity instead.

namespace DEM::Game
{
typedef Ptr<class CGameLevel> PGameLevel;

struct CEntity final
{
	PGameLevel Level; // Is never nullptr for a valid entity. Also gives access to the world.
	CStrID     Name;  // Useful for editors, debug and scripting. Can be used in serialization.
	bool       IsActive = true; // TODO: lifecycle flags here too
};

// NB: can't store HEntity inside a CEntity (but could store uint32_t if necessary)
using CEntityStorage = Data::CHandleArray<CEntity, uint32_t, 18, true>;
using HEntity = CEntityStorage::CHandle;

}

namespace std
{

template<>
struct hash<DEM::Game::HEntity>
{
	size_t operator()(const DEM::Game::HEntity _Keyval) const noexcept
	{
		return static_cast<size_t>(Hash(_Keyval));
	}
};

}