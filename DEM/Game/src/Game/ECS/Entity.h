#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/HandleArray.h>
#include <Data/Metadata.h>

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
	CStrID     LevelID;         // Is never empty for a valid entity. Cached level ptr gives access to the world.
	CStrID     TemplateID;      // Empty if this entity is not created from a template
	CStrID     Name;            // Useful for editors, debug and scripting. Can be used in serialization.
	PGameLevel Level;
	bool       IsActive = true; // TODO: lifecycle flags here too
};

// NB: can't store HEntity inside a CEntity (but could store uint32_t if necessary)
using CEntityStorage = Data::CHandleArray<CEntity, uint32_t, 18, true>;
using HEntity = CEntityStorage::CHandle;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CEntity>() { return "DEM::Game::CEntity"; }
template<> inline constexpr auto RegisterMembers<Game::CEntity>()
{
	return std::make_tuple
	(
		Member(1, "LevelID", &Game::CEntity::LevelID, &Game::CEntity::LevelID),
		Member(2, "TemplateID", &Game::CEntity::TemplateID, &Game::CEntity::TemplateID),
		Member(3, "Name", &Game::CEntity::Name, &Game::CEntity::Name),
		Member(4, "IsActive", &Game::CEntity::IsActive, &Game::CEntity::IsActive)
	);
}

}

namespace std
{

template<>
struct hash<DEM::Game::HEntity>
{
	size_t operator()(const DEM::Game::HEntity _Keyval) const noexcept
	{
		return static_cast<size_t>(Hash(_Keyval));

		// TODO: more profiling required
		// The best hash function returns different value for each input.
		// All alive entities have different index bits by design. Hashing
		// them by their index bits shows almost the same result as Hash().
		//return static_cast<size_t>(_Keyval.Raw & ((1 << 18) - 1));
	}
};

}
