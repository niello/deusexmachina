#pragma once
#include <Scripting/SolLow.h>
#include <Game/ECS/Entity.h>

// Wrapper for Sol header with template overrides required for DEM Game layer

namespace DEM::Game
{
class CGameWorld;
}

namespace sol
{
// FIXME: sol 3.3.0 - we may want explicit T& to be passed by ref, but sol checks trait for T& when passing T
template <> struct is_value_semantic_for_function<DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<DEM::Game::HEntity&> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity&> : std::true_type {};
}

namespace DEM::Scripting
{
void RegisterGameTypes(sol::state& State, Game::CGameWorld& World);
}
