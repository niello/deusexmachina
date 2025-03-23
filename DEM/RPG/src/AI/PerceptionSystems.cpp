#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <AI/Perception.h>
#include <AI/AIStateComponent.h>
#include <AI/VisionSensorComponent.h>

namespace DEM::Game
{
	bool GetTargetFromPhysicsObject(const Physics::CPhysicsObject& Object, CTargetInfo& OutTarget);
}

namespace DEM::RPG
{

void SenseVisualStimulus(Game::CGameSession& Session, Game::HEntity SensorID, Game::HEntity StimulusID, float Modifier, AI::EAwareness& OutAwareness, uint8_t& OutTypeFlags)
{
	// TODO: process

	// get visibile component and use visibility coeff/state/modifier from there

	// here is a detection check for invisible and hidden objects/characters against the sensor

	// choose an awareness level based on Modifier and detection

	// can apply sensor cheats and buffs here

	OutAwareness = AI::EAwareness::Full;
	// OutTypeFlags = ...
}
//---------------------------------------------------------------------

// TODO: move common logic to DEMGame as utility function(s)
void ProcessVisionSensors(Game::CGameSession& Session, Game::CGameWorld& World, Game::CGameLevel& Level)
{
	World.ForEachEntityInLevelWith<const AI::CVisionSensorComponent, AI::CAIStateComponent>(Level.GetID(),
		[&Session, &Level](auto SensorID, auto& Entity, const AI::CVisionSensorComponent& Sensor, AI::CAIStateComponent& AIState)
	{
		if (!Sensor.Node || Sensor.MaxRadius <= 0.f) return; // continue

		ZoneScopedN("VisionSensor");

		// skip by sparse update, sensors must be distributed more or less evenly among frames

		//???separate collision world and simplified shapes for sensing? or add sensing collision flags to physics bodies and existing colliders?

		// TODO PERF: could reuse shape and collision flags and perform contactTest manually, but is it really benefical?
		Level.EnumEntitiesInSphere(Sensor.Node->GetWorldPosition(), Sensor.MaxRadius, /* "Visible"sv */ "Dynamic"sv,
			[&Session, &Level, &Sensor, SensorID, &AIState](Game::HEntity StimulusID, const rtm::vector4f& ContactPos)
		{
			if (StimulusID == SensorID) return true; // continue

			const auto& SensorTfm = Sensor.Node->GetWorldMatrix();
			const auto SensorPos = SensorTfm.w_axis;
			const auto SensorToContact = rtm::vector_sub(ContactPos, SensorPos);

			// Test against max vision distance
			// FIXME: why physics system reports such contacts?
			const float DistanceToContactSq = rtm::vector_length_squared3(SensorToContact);
			if (DistanceToContactSq > Sensor.MaxRadiusSq) return true; // continue

			// Test against max vision angle
			const auto LookatDir = rtm::vector_normalize3(rtm::vector_neg(SensorTfm.z_axis));
			const auto ContactDir = rtm::vector_mul(SensorToContact, rtm::scalar_sqrt_reciprocal(DistanceToContactSq));
			const float CosLookAtStimulus = rtm::vector_dot3(LookatDir, ContactDir);
			if (CosLookAtStimulus < Sensor.CosHalfMaxFOV) return true; // continue

			// Modify stimulus intensity with peripheral vision and distance coefficients
			float Modifier = 1.f;
			if (CosLookAtStimulus < Sensor.CosHalfPerfectFOV)
				Modifier *= 1.f - (Sensor.CosHalfPerfectFOV - CosLookAtStimulus) / (Sensor.CosHalfPerfectFOV - Sensor.CosHalfMaxFOV);
			if (DistanceToContactSq > Sensor.PerfectRadiusSq)
				Modifier *= 1.f - (rtm::scalar_sqrt(DistanceToContactSq) - Sensor.PerfectRadius) / (Sensor.MaxRadius - Sensor.PerfectRadius);
			n_assert_dbg(Modifier > 0.f);

			// Apply game logic
			AI::EAwareness Awareness = AI::EAwareness::None;
			uint8_t TypeFlags = 0;
			RPG::SenseVisualStimulus(Session, SensorID, StimulusID, Modifier, Awareness, TypeFlags);
			if (Awareness == AI::EAwareness::None) return true; // continue

			// Perform line of sight test
			// TODO PERF:
			//   can use navmesh raycast as a cheap first chance check, must work good with walls etc
			//   can offset raycast to target side if it is moving, or try multiple raycasts to sides and upper/lower parts
			//   can do raycast async, but wait for results if this stimulus is considered by decision making? Or when merging starts?
			//   raycast can be performed less frequently then a sensor test, especially if the object has passed a raycast test before
			//   may do async raycasts for objects with previous successfull raycast, i.e. currently visible ones
			//   must not filter out characters standing one after another! Check only static geometry and dynamic things like doors?
			if (auto* pPhysicsObject = Level.GetFirstPickIntersection(SensorPos, ContactPos, nullptr, /*"Visible|Static|Dynamic"sv*/ ""sv, SensorID))
			{
				Game::CTargetInfo LOSCollision;
				Game::GetTargetFromPhysicsObject(*pPhysicsObject, LOSCollision);
				if (LOSCollision.Entity != StimulusID) return true; // continue
			}

			// Register a new stimulus in the AI brain
			auto& Stimulus = AIState.NewStimuli.emplace_back();
			Stimulus.Position = ContactPos;
			Stimulus.SourceID = StimulusID;
			//Stimulus.AddedTimestamp - here or later when merging?
			//Stimulus.UpdatedTimestamp - here or later when merging?
			Stimulus.Awareness = Awareness;
			Stimulus.TypeFlags = TypeFlags;
			Stimulus.ModalityFlags = static_cast<uint8_t>(AI::ESenseModality::Vision);

			return true; // continue
		});
	});
}
//---------------------------------------------------------------------

}
