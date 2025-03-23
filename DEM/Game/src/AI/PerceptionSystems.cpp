#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>
#include <AI/VisionSensorComponent.h>

namespace DEM::AI
{

void InitCharacterAI(Game::CGameWorld& World)
{
	World.ForEachEntityWith<CVisionSensorComponent, const Game::CSceneComponent>(
		[](auto EntityID, auto& Entity, CVisionSensorComponent& Sensor, const Game::CSceneComponent& Scene)
	{
		Sensor.Node = Scene.RootNode->FindNodeByPath(Sensor.NodePath.c_str());
		Sensor.CosHalfMaxFOV = std::cosf(n_deg2rad(Sensor.MaxFOV) * 0.5f);
		Sensor.CosHalfPerfectFOV = std::cosf(n_deg2rad(Sensor.PerfectFOV) * 0.5f);
	});
}
//---------------------------------------------------------------------

}
