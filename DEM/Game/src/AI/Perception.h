#pragma once

// Common structures and utils for AI perception

namespace DEM::AI
{

enum EStimulusType // flags
{
	Unspecified = 0,
	Threat,
	Danger,
	Movement,
	Contact,
	Anomaly,
	Benefit
};

enum class EAwareness
{
	None = 0,
	Faint,    // The fact of existence
	Weak,     // Approximate direction, type of sound
	Moderate, // Exact direction, approximate position, movement presense, language
	Strong,   // Exact position, movement direction and speed, broad physical features, general meaning
	Detailed, // Detailed appearance, nuances
	Full      // Perfect awareness
};

}
