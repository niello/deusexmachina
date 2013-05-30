#pragma once
#ifndef __DEM_L1_ANIM_CTLR_RIGID_BODY_H__
#define __DEM_L1_ANIM_CTLR_RIGID_BODY_H__

#include <Scene/NodeController.h>

// Animation controller, that samples world transform from a physically simulated rigid body.

class btRigidBody;

namespace Anim
{

class CNodeControllerRigidBody: public Scene::CNodeController
{
protected:

	btRigidBody*	pRB;
	bool			TfmChanged;

public:

	CNodeControllerRigidBody(): pRB(NULL), TfmChanged(false) { }

	//???on deactivation forceActivationState(DISABLE_SIMULATION) or let the object be simulated?
	//???ctlr creates and owns body, or even pass motion state here?
	//!!!override motion state, default one isn't good!
	//controller body must have motion state which calls back to the controller
	void			SetBody(btRigidBody* pRigidBody);
	void			Clear();
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerRigidBody> PNodeControllerRigidBody;

}

#endif
