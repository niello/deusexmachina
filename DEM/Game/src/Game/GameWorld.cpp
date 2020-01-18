#include "GameWorld.h"
#include <Game/GameLevel.h>

namespace DEM::Game
{

// FIXME: make difference between 'non-interactive' and 'interactive same as whole'. AABB::Empty + AABB::Invalid?
CGameLevel* CGameWorld::CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds, UPTR SubdivisionDepth)
{
	// Ensure there is no level with same ID

	auto Level = n_new(DEM::Game::CGameLevel(ID, Bounds, InteractiveBounds, SubdivisionDepth));

	// Add level to the list

	// Notify level activated?

	// Return level ptr

	return nullptr;
}
//---------------------------------------------------------------------

CGameLevel* CGameWorld::LoadLevel(CStrID ID /*CParams or delegate, must include changes if not fresh copy*/)
{
	// Create level, using its description
	// Load static scn, if present (includes static collision in .bullet resource)
	// Load navigation map, if present

	// For each entity
	//   Create entity
	//   Create components
	//   Load values to components
	//   Attach entity to the level

	// Notify level loaded / activated, validate if automatic

	// Return level ptr

	return nullptr;
}
//---------------------------------------------------------------------

}