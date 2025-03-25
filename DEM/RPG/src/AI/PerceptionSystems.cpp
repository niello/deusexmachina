#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <AI/Perception.h>
#include <AI/AIStateComponent.h>
#include <AI/VisionSensorComponent.h>
#include <AI/SoundSensorComponent.h>
#include <AI/AILevel.h>

namespace DEM::Game
{
	bool GetTargetFromPhysicsObject(const Physics::CPhysicsObject& Object, CTargetInfo& OutTarget);
}

namespace DEM::RPG
{
constexpr size_t MAX_STIMULI_PER_TICK = 128;

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

void SenseSoundStimulus(Game::CGameSession& Session, Game::HEntity SensorID, const AI::CStimulusEvent& StimulusEvent, float IntensityAtSensor, AI::EAwareness& OutAwareness, uint8_t& OutTypeFlags)
{
	// TODO: process

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
			if (AIState.NewStimuli.size() >= MAX_STIMULI_PER_TICK) return false; // break

			if (StimulusID == SensorID) return true; // continue

			const auto& SensorTfm = Sensor.Node->GetWorldMatrix();
			const auto SensorPos = SensorTfm.w_axis;
			const auto FromSensorToContact = rtm::vector_sub(ContactPos, SensorPos);

			// Test against max vision distance
			// FIXME: why physics system reports such contacts?
			const float DistanceToContactSq = rtm::vector_length_squared3(FromSensorToContact);
			if (DistanceToContactSq > Sensor.MaxRadiusSq) return true; // continue

			// Test against max vision angle
			const auto LookatDir = rtm::vector_normalize3(rtm::vector_neg(SensorTfm.z_axis));
			const auto ContactDir = rtm::vector_mul(FromSensorToContact, rtm::scalar_sqrt_reciprocal(DistanceToContactSq));
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
			if (auto* pLOSBlocker = Level.GetFirstPickIntersection(SensorPos, ContactPos, nullptr, /*"Visible|Static|Dynamic"sv*/ ""sv, SensorID))
			{
				Game::CTargetInfo LOSCollision;
				Game::GetTargetFromPhysicsObject(*pLOSBlocker, LOSCollision);
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
			Stimulus.ModalityFlags = (1 << static_cast<uint8_t>(AI::ESenseModality::Vision));

			return true; // continue
		});
	});
}
//---------------------------------------------------------------------

// TODO: can set in AI level or even vary at different points of the level
constexpr float SoundAttenuationCoeff = 0.05f;

// TODO: must set in AI manager
constexpr float LowestUsefulIntensity = 0.01f;

// TODO: move common logic to DEMGame as utility function(s)
void ProcessSoundSensors(Game::CGameSession& Session, Game::CGameWorld& World, Game::CGameLevel& Level)
{
	if (!Level.GetAI()) return;

	//!!!DBG TMP!
	{
		AI::CStimulusEvent DBGEvent;
		DBGEvent.Position = rtm::vector_set(113.f, 38.1f, 113.f);
		DBGEvent.SourceID = Game::HEntity{ 9 };
		DBGEvent.Intensity = 0.3f;
		DBGEvent.TypeFlags = (1 << static_cast<uint8_t>(AI::EStimulusType::Benefit));
		DBGEvent.Modality = AI::ESenseModality::Sound;
		Level.GetAI()->AddStimulusEvent(DBGEvent);
	}

	Level.GetAI()->ProcessStimulusEvents([&Session, &World, &Level](const AI::CStimulusEvent& StimulusEvent)
	{
		//!!!FIXME: need switch by type and a separate processing function per type!
		n_assert(StimulusEvent.Modality == AI::ESenseModality::Sound);

		// TODO: or set manually when registering events?
		//!!!can do LowestUsefulIntensity += StimulusMasking from environment if it is constant across the level (or use the lowest masking if not!)
		// this will significantly reduce the collision radius and skip all too silent sounds immediately!
		if (StimulusEvent.Intensity <= LowestUsefulIntensity) return; // continue
		n_assert_dbg(LowestUsefulIntensity > 0.f && SoundAttenuationCoeff > 0.f);
		const float MaxRadius = rtm::scalar_sqrt((StimulusEvent.Intensity - LowestUsefulIntensity) / (LowestUsefulIntensity * SoundAttenuationCoeff));

		// TODO: can collide only passive sensors of the corresponding modality, need a separate collision flag
		Level.EnumEntitiesInSphere(StimulusEvent.Position, MaxRadius, /* "SoundSensor"sv */ "Dynamic"sv,
			[&Session, &World, &Level, &StimulusEvent](Game::HEntity SensorID, const rtm::vector4f& ContactPos)
		{
			const auto* pSoundSensor = World.FindComponent<const AI::CSoundSensorComponent>(SensorID);
			if (!pSoundSensor) return true; // continue

			const auto FromSoundToSensor = rtm::vector_sub(ContactPos, StimulusEvent.Position);
			const float DistanceSq = rtm::vector_length_squared3(FromSoundToSensor);

			float IntensityAtSensor = StimulusEvent.Intensity / (1.f + SoundAttenuationCoeff * DistanceSq);
			if (IntensityAtSensor < pSoundSensor->IntensityThreshold) return true; // continue

			IntensityAtSensor -= Level.GetAI()->GetStimulusMaskingAt(AI::ESenseModality::Sound, ContactPos);
			if (IntensityAtSensor < pSoundSensor->IntensityThreshold) return true; // continue

			// TODO: do intensity reduction by raycast or pathfinding, if needed (and maybe delay increase and even pos modification)

			// TODO: if delay needed, must add the stimulus in the future, and skip it during merge until its time is passed (can use separate array or std::partition)

			// Apply game logic
			AI::EAwareness Awareness = AI::EAwareness::None;
			uint8_t TypeFlags = StimulusEvent.TypeFlags;
			RPG::SenseSoundStimulus(Session, SensorID, StimulusEvent, IntensityAtSensor, Awareness, TypeFlags);
			if (Awareness == AI::EAwareness::None) return true; // continue

			// Register a new stimulus in the AI brain
			auto* pAIState = World.FindComponent<AI::CAIStateComponent>(SensorID);
			if (pAIState && pAIState->NewStimuli.size() < MAX_STIMULI_PER_TICK)
			{
				auto& Stimulus = pAIState->NewStimuli.emplace_back();
				Stimulus.Position = StimulusEvent.Position;
				Stimulus.SourceID = StimulusEvent.SourceID;
				//Stimulus.AddedTimestamp - here or later when merging?
				//Stimulus.UpdatedTimestamp - here or later when merging?
				Stimulus.Awareness = Awareness;
				Stimulus.TypeFlags = TypeFlags;
				Stimulus.ModalityFlags = (1 << static_cast<uint8_t>(AI::ESenseModality::Sound));
			}

			return true; // continue
		});
	});
}
//---------------------------------------------------------------------

}
