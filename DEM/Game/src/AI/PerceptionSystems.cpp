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
		if (Sensor.MaxFOV < Sensor.PerfectFOV) Sensor.MaxFOV = Sensor.PerfectFOV;
		if (Sensor.MaxRadius < Sensor.PerfectRadius) Sensor.MaxRadius = Sensor.PerfectRadius;

		Sensor.Node = Scene.RootNode->FindNodeByPath(Sensor.NodePath.c_str());
		Sensor.PerfectRadiusSq = Sensor.PerfectRadius * Sensor.PerfectRadius;
		Sensor.MaxRadiusSq = Sensor.MaxRadius * Sensor.MaxRadius;
		Sensor.CosHalfPerfectFOV = std::cosf(n_deg2rad(Sensor.PerfectFOV) * 0.5f);
		Sensor.CosHalfMaxFOV = std::cosf(n_deg2rad(Sensor.MaxFOV) * 0.5f);
	});
}
//---------------------------------------------------------------------

}
