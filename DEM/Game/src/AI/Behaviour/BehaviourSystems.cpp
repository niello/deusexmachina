#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <AI/Behaviour/BehaviourTreeComponent.h>
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <AI/AIStateComponent.h>

namespace DEM::AI
{

void InitCharacterAIThinking(Game::CGameWorld& World, Game::CGameSession& Session, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CBehaviourTreeComponent>([&ResMgr, &World, &Session](auto EntityID, CBehaviourTreeComponent& Component)
	{
		ResMgr.RegisterResource<CBehaviourTreeAsset>(Component.Asset);
		if (!Component.Asset) return;

		if (auto* pBTAsset = Component.Asset->ValidateObject<CBehaviourTreeAsset>())
		{
			Component.Player.SetAsset(pBTAsset);

			if (auto* pBrain = World.FindComponent<CAIStateComponent>(EntityID))
				Component.Player.Start(Session, EntityID);
		}
	});
}
//---------------------------------------------------------------------

void UpdateBehaviourTrees(DEM::Game::CGameWorld& World, float dt)
{
	World.ForEachComponent<CBehaviourTreeComponent>([dt](auto EntityID, CBehaviourTreeComponent& Component)
	{
		if (Component.Player.IsPlaying())
			Component.Player.Update(dt);
	});
}
//---------------------------------------------------------------------

}
