#pragma once
#ifndef __DEM_L2_PROP_ABSTRACT_PHYSICS_H__
#define __DEM_L2_PROP_ABSTRACT_PHYSICS_H__

#include "PropTransformable.h"
#include <mathlib/bbox.h>

// Abstract Physics property, provides a enable and disable mechanism.
// Based on mangalore AbstractPhysicsProperty_(C) 2005 Radon Labs GmbH

namespace Physics
{
	class CEntity;
};

namespace Properties
{

class CPropAbstractPhysics: public CPropTransformable
{
	__DeclareClassNoFactory;

protected:

	bool Enabled;

	virtual void EnablePhysics();
	virtual void DisablePhysics();
	virtual void SetTransform(const matrix44& NewTF);

public:

	CPropAbstractPhysics(): Enabled(false) {}
	virtual ~CPropAbstractPhysics();

	virtual void				Activate();
	virtual void				Deactivate();

	void						SetEnabled(bool Enable);
	bool						IsEnabled() const { return Enabled; }

	virtual Physics::CEntity*	GetPhysicsEntity() const = 0;
	void						GetAABB(bbox3& AABB) const;
};

}

#endif
