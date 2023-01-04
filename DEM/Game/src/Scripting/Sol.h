#pragma once
#include <sol/sol.hpp>
#include <Game/ECS/Entity.h>

// Wrapper for Sol header with template overrides required for DEM

namespace sol
{
// FIXME: sol 3.3.0 - we may want explicit T& to be passed by ref, but sol checks trait for T& when passing T
template <> struct is_value_semantic_for_function<DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity> : std::true_type {};
template <> struct is_value_semantic_for_function<DEM::Game::HEntity&> : std::true_type {};
template <> struct is_value_semantic_for_function<const DEM::Game::HEntity&> : std::true_type {};
template <> struct is_value_semantic_for_function<CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<CStrID&> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID&> : std::true_type {};
}
