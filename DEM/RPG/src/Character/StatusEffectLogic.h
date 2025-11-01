#include <Character/StatusEffect.h>

// ECS systems and utils for character status effect application and management

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{

bool AddStatusEffect(Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, const CStatusEffectInstance& Instance);

}
