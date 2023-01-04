#pragma once
#include <sol/sol.hpp>
#include <Game/ECS/Entity.h>

// Wrapper for Sol header with template overrides required for DEM

namespace sol
{
template <> struct is_value_semantic_for_function<DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<DEM::Game::HEntity&> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity&> : std::true_type {};
}
