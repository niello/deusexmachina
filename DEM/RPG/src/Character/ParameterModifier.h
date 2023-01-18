#pragma once
#include <StdDEM.h>

// A modifier that can be attached to a parameter of a character or an object to affect its
// final value. All active modifiers of a single parameter are stored as a forward linked list.

namespace DEM::RPG
{

template<typename T>
class CParameterModifier
{
public:

	std::unique_ptr<CParameterModifier<T>> NextModifier;

	virtual ~CParameterModifier() = default;

	virtual bool Apply(T& Value) = 0; // Returns false if the modifier has expired
};

}
