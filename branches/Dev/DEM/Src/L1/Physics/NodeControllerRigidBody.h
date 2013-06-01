#pragma once
#ifndef __DEM_L1_NODE_CTLR_RIGID_BODY_H__
#define __DEM_L1_NODE_CTLR_RIGID_BODY_H__

#include <Scene/NodeController.h>
#include <Physics/RigidBody.h>

// Animation controller, that samples world transform from a physically simulated rigid body.

namespace Physics
{

class CNodeControllerRigidBody: public Scene::CNodeController
{
protected:

	PRigidBody	Body;
	bool		TfmChanged;	//!!!to motion state!

public:

	CNodeControllerRigidBody(): TfmChanged(false) { }

	//???on deactivation forceActivationState(DISABLE_SIMULATION) or let the object be simulated?
	//???ctlr creates and owns body, or even pass motion state here?
	//!!!override motion state, default one isn't good!
	//controller body must have motion state which calls back to the controller
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerRigidBody> PNodeControllerRigidBody;

}

#endif
