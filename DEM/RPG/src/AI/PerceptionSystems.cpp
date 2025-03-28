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
constexpr float SoundAttenuationCoeff = 0.05f; // TODO: can set in AI level or even vary at different points of the level
constexpr float LowestUsefulIntensity = 0.01f; // TODO: must set in AI manager

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

// TODO: hardcode appropriate values or get them from settings
static inline bool ForgetFact(AI::CSensedStimulus& Fact, uint32_t CurrTimestamp, float PersonalModifier = 1.f)
{
	ZoneScoped;

	const float TimeSinceLastSensed = (CurrTimestamp - Fact.UpdatedTimestamp) * PersonalModifier;

	if (Fact.Awareness <= AI::EAwareness::Faint)
	{
		// A simple fact of knowing that something exists becomes unimportant
		return TimeSinceLastSensed > 3.f;
	}
	else if ((Fact.TypeFlags & static_cast<uint8_t>(AI::EStimulusType::Movement)) && TimeSinceLastSensed > 2.f)
	{
		// A location of a moving stimulus quickly loses actuality
		Fact.Awareness = AI::EAwareness::Faint;
		Fact.UpdatedTimestamp = CurrTimestamp;
		return false;
	}
	else if (Fact.Awareness >= AI::EAwareness::Strong)
	{
		// Strong knowledge fades away slower
		return TimeSinceLastSensed > 5.f;
	}
	else
	{
		// And weak one does that pretty fast
		return TimeSinceLastSensed > 2.f;
	}
}
//---------------------------------------------------------------------

static inline AI::EAwareness MergeAwareness(AI::EAwareness a, AI::EAwareness b)
{
	// Full awareness is special, it can't be reached by combining information from multiple stimuli
	if (a == b && a < AI::EAwareness::Detailed)
		return static_cast<AI::EAwareness>(static_cast<uint8_t>(a) + 1);
	else
		return std::max(a, b);
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
		Level.EnumEntitiesInSphere(Sensor.Node->GetWorldPosition(), Sensor.MaxRadius, /* "Visible"sv */ ""sv,
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

// TODO: move common logic to DEMGame as utility function(s)
void ProcessSoundSensors(Game::CGameSession& Session, Game::CGameWorld& World, Game::CGameLevel& Level)
{
	if (!Level.GetAI()) return;

	ZoneScoped;

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

		ZoneScopedN("SoundStimulus");

		n_assert_dbg(LowestUsefulIntensity > 0.f && SoundAttenuationCoeff > 0.f);
		const float MaxRadius = rtm::scalar_sqrt((StimulusEvent.Intensity - LowestUsefulIntensity) / (LowestUsefulIntensity * SoundAttenuationCoeff));

		// TODO: can collide only passive sensors of the corresponding modality, need a separate collision flag
		// TODO: OR can provide a needCollision delegate and filter out entities without CSoundSensorComponent before collision check, but is slow part after this check?
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

void MergeAIMemory(Game::CGameWorld& World)
{
	//!!!DBG TMP! Need a reusable buffer in an AI system!
	std::vector<AI::CSensedStimulus> MergedFacts;

	// TODO: need time e.g. in msec from game start, enough for 46 days. Or in AI ticks for memory fact forgetting?
	//!!!DBG TMP!
	static uint32_t Timer = 0;
	uint32_t CurrTimestamp = ++Timer;

	World.ForEachComponent<AI::CAIStateComponent>([&MergedFacts, CurrTimestamp](auto EntityID, AI::CAIStateComponent& AIState)
	{
		//!!!TODO PERF: sparse update! AI ticks and dividing to subsequent frames for an even workload.

		ZoneScopedN("MergeAIMemory");

		if (AIState.NewStimuli.empty() && AIState.Facts.empty()) return; // continue

		MergedFacts.clear();
		MergedFacts.reserve(std::max(AIState.NewStimuli.size(), AIState.Facts.size()));

		// Shift sourceless stimuly to the end of the range, sort sourceful ones by source ID
		const auto ItNewBegin = AIState.NewStimuli.begin();
		auto ItNewEnd = std::partition(ItNewBegin, AIState.NewStimuli.end(), [](const AI::CSensedStimulus& Fact) { return Fact.SourceID; });
		std::sort(ItNewBegin, ItNewEnd, [](const AI::CSensedStimulus& a, const AI::CSensedStimulus& b) { return a.SourceID < b.SourceID; });

		auto ItNew = ItNewBegin;
		bool IsEndNew = (ItNew == ItNewEnd);

		auto ItCurr = AIState.Facts.begin();
		auto ItCurrEnd = ItCurr + AIState.FactWithSourceCount;
		bool IsEndCurr = (ItCurr == ItCurrEnd);

		// Merge existing facts and new stimuli into a new memory state, only for records with sources defined
		while (!IsEndNew || !IsEndCurr)
		{
			if (IsEndCurr || (!IsEndNew && ItNew->SourceID < ItCurr->SourceID))
			{
				// Only new stimulus, simply add
				auto& MergedFact = MergedFacts.emplace_back(*ItNew);
				MergedFact.AddedTimestamp = CurrTimestamp;
				MergedFact.UpdatedTimestamp = CurrTimestamp;
				IsEndNew = (++ItNew == ItNewEnd);
			}
			else if (IsEndNew || ItCurr->SourceID < ItNew->SourceID)
			{
				// Only old stimulus, apply forgetting and discard if totally forgotten
				if (!ForgetFact(*ItCurr, CurrTimestamp))
					MergedFacts.push_back(*ItCurr);
				IsEndCurr = (++ItCurr == ItCurrEnd);
			}
			else // equal
			{
				// Update the existing stimulus with new info
				auto& MergedFact = MergedFacts.emplace_back(*ItCurr);
				MergedFact.UpdatedTimestamp = CurrTimestamp;
				IsEndCurr = (++ItCurr == ItCurrEnd);

				do
				{
					if (ItNew->Awareness > AI::EAwareness::Faint && ItNew->Awareness >= MergedFact.Awareness)
						MergedFact.Position = ItNew->Position;
					MergedFact.Awareness = MergeAwareness(ItNew->Awareness, MergedFact.Awareness);
					MergedFact.TypeFlags = (ItNew->TypeFlags | MergedFact.TypeFlags);
					MergedFact.ModalityFlags = (ItNew->ModalityFlags | MergedFact.ModalityFlags);

					IsEndNew = (++ItNew == ItNewEnd);
				}
				while (!IsEndNew && ItNew->SourceID == MergedFact.SourceID);
			}
		}

		// Remember the count before adding sourceless facts
		AIState.FactWithSourceCount = MergedFacts.size();

		// Merge sourceless records by proximity, first new, then existing.
		// What haven't been merged is saved as separate facts /*TODO: but within a limit*/.
		//!!!TODO: merge by proximity! use k-d tree like nanoflann? or grid spatial hash? or naive O(m*n) is ok for our case with small element count?
		for (; ItNew != AIState.NewStimuli.end(); ++ItNew)
		{
			MergedFacts.push_back(*ItNew);
		}
		for (; ItCurr != AIState.Facts.end(); ++ItCurr)
		{
			MergedFacts.push_back(*ItCurr);
		}

		// Store a new memory state in the AI brain
		// TODO PERF: can prevent all buffers to grow to the size of the biggest one among all actors? May copy contents instead of swap, but it is slower!
		std::swap(AIState.Facts, MergedFacts);
		AIState.NewStimuli.clear();
	});
}
//---------------------------------------------------------------------

}
