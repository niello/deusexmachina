#pragma once
#include <StdDEM.h>

// Abstract interface for acceptable target filtering. Examines target info
// and returns whether it can be used for the interaction.

namespace DEM::Game
{
struct CInteractionContext;

class ITargetFilter
{
public:

	static inline constexpr auto CURRENT_TARGET = std::numeric_limits<U32>().max();

	virtual bool IsTargetValid(const CInteractionContext& Context, U32 Index = CURRENT_TARGET) const = 0;
};

}
