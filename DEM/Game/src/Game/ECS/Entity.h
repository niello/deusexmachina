#pragma once
#include <Data/Ptr.h>
#include <Data/HandleArray.h>
#include <Data/SerializeToParams.h>

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
	CStrID LevelID;         // Is never empty for a valid entity. Cached level ptr gives access to the world.
	CStrID TemplateID;      // Empty if this entity is not created from a template
	CStrID Name;            // Useful for editors, debug and scripting. Can be used in serialization.
	bool   IsActive = true;
};

// NB: can't store HEntity inside a CEntity (but could store uint32_t if necessary)
using CEntityStorage = Data::CHandleArray<CEntity, uint32_t, 18, true>;
using HEntity = CEntityStorage::CHandle;

//!!!FIXME: use template specializations for ToString?!
inline std::string EntityToString(HEntity EntityID)
{
	return std::to_string(EntityID.Raw & CEntityStorage::INDEX_BITS_MASK) + 'v' +
		std::to_string(EntityID.Raw >> CEntityStorage::INDEX_BITS);
}

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

namespace DEM::Serialization
{

template<>
struct ParamsFormat<DEM::Game::HEntity>
{
	static inline void Serialize(Data::CData& Output, DEM::Game::HEntity Value)
	{
		static_assert(sizeof(DEM::Game::HEntity::TRawValue) <= sizeof(int), "Entity ID data may be partially lost");

		if (Value)
			Output = Value.Raw;
		else
			Output.Clear();
	}

	static inline void Deserialize(const Data::CData& Input, DEM::Game::HEntity& Value)
	{
		Value = Input.IsVoid() ?
			DEM::Game::HEntity{} :
			DEM::Game::HEntity{ static_cast<decltype(Value.Raw)>(Input.GetValue<int>()) };
	}
};

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
