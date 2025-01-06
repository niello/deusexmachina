#pragma once
#include <Data/RefCounted.h>

// A modifier that can be attached to a parameter of a character or an object to affect its
// final value. All active modifiers of a single parameter are stored as a forward linked list.

// A modifier is active only while it is connected to both source and receiver of the modification.
// Destruction of one or another will lead to destruction of the modifier. That's why the
// modifier has to be refcounted. In alive state it has at least 2 references. As soon as refcount
// drops below this value, the modifier will be destroyed at the first opportunity.

namespace DEM::RPG
{

template<typename T>
class CParameterModifier : public Data::CRefCounted
{
public:

	using TParam = T;

	Ptr<CParameterModifier<T>> NextModifier;

	virtual ~CParameterModifier() = default;

	virtual I32  GetPriority() const = 0; // Higher priority modifiers applied first
	virtual bool Apply(T& Value) = 0;     // Returns false if the modifier has expired
	bool         IsConnected() const { return GetRefCount() > 1; }
};

}
