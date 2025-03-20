#pragma once
#include <Game/ECS/Entity.h>

// Common structures and utils for AI perception

namespace DEM::AI
{

enum EStimulusType : uint8_t // flags
{
	Unspecified = 0,
	Threat,
	Danger,
	Movement,
	Contact,
	Anomaly,
	Benefit
};

enum class EAwareness : uint8_t
{
	None = 0,
	Faint,    // The fact of existence
	Weak,     // Approximate direction, type of sound
	Moderate, // Exact direction, approximate position, movement presense, sound tone, speech language
	Strong,   // Exact position, movement direction and speed, broad physical features, general meaning
	Detailed, // Detailed appearance, nuances
	Full      // Perfect awareness
};

struct CSensedStimulus
{
	rtm::vector4f Position;
	Game::HEntity SourceID;
	uint32_t      AddedTimestamp;
	uint32_t      UpdatedTimestamp;
	EAwareness    Awareness;
	uint8_t       TypeFlags; // EStimulusType
};

}
