#pragma once
#include <Game/ECS/Entity.h>

// Common structures and utils for AI perception

namespace DEM::AI
{

enum class EStimulusType: uint8_t
{
	Threat,
	Danger,
	Movement,
	Contact,
	Anomaly,
	Benefit
};

enum class ESenseModality : uint8_t
{
	Vision,
	Sound,
	Smell,
	Touch,
	SelfAwareness,

	Count,
	Unspecified
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

struct CStimulusEvent
{
	rtm::vector4f  Position;
	Game::HEntity  SourceID;
	float          Intensity = 1.f;
	uint8_t        TypeFlags = 0; // EStimulusType
	ESenseModality Modality = ESenseModality::Unspecified;
};

struct CSensedStimulus
{
	rtm::vector4f Position;
	Game::HEntity SourceID;
	uint32_t      AddedTimestamp;
	uint32_t      UpdatedTimestamp;
	EAwareness    Awareness;
	uint8_t       TypeFlags;        // EStimulusType
	uint8_t       ModalityFlags;    // EStimulusModality
};

}
