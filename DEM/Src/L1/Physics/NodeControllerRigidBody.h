#pragma once
#ifndef __DEM_L1_NODE_CTLR_RIGID_BODY_H__
#define __DEM_L1_NODE_CTLR_RIGID_BODY_H__

#include <Scene/NodeController.h>
#include <Physics/RigidBody.h>

// Scene node controller, that samples world transform from a physically simulated rigid body.

namespace Physics
{

class CNodeControllerRigidBody: public Scene::CNodeController
{
protected:

	PRigidBody	Body;

public:

	//???on deactivation forceActivationState(DISABLE_SIMULATION) or let the object be simulated?

	void			SetBody(CRigidBody& RigidBody);
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerRigidBody> PNodeControllerRigidBody;

}

#endif
