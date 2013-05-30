#pragma once
#ifndef __DEM_L2_CAMERA_MANAGER_H__
#define __DEM_L2_CAMERA_MANAGER_H__

//#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Scene/NodeControllerThirdPerson.h>
//#include <Scene/Camera.h>
//#include <util/npfeedbackloop.h>

// Manages the main camera of the active level. Provides interface for driving camera
// (manually, by input, by script), performing smooth transition between changing targets,
// chasing or tracking some scene node, shaking to simulate explosions and quakes etc.
// Camera manager also takes care of camera collisions and target size dependent limiting.

//???autoadjust camera aspect here or in scene?

//!!!L2 CameraManager:
// Third-person COI rotation with 2-sided vertical angle limitimg
// COI moving
// COI from scene node (chase/track)
// Smooth move from one to another target, changing COI and position
// Control by UI and API
// Set camera (which camera to use)
// Focus on entity - smooth move from curr state to new object

//BEGIN_ATTRS_REGISTRATION(PropChaseCamera)
//	RegisterFloatWithDefault(CameraDistance, ReadOnly, 7.0f);
//	RegisterFloatWithDefault(CameraMinDistance, ReadOnly, 1.5f);
//	RegisterFloatWithDefault(CameraMaxDistance, ReadOnly, 15.0f);
//	RegisterFloatWithDefault(CameraAngularVelocity, ReadOnly, 6.0f);
//	RegisterVector3WithDefault(CameraOffset, ReadOnly, vector4(0.0f, 1.5f, 0.0f, 0.f));
//	RegisterFloatWithDefault(CameraLowStop, ReadOnly, 5.0f);
//	RegisterFloatWithDefault(CameraHighStop, ReadOnly, 45.0f);
//	RegisterFloatWithDefault(CameraDistanceStep, ReadOnly, 1.0f);
//	RegisterFloatWithDefault(CameraLinearGain, ReadOnly, -10.0f);
//	RegisterFloatWithDefault(CameraAngularGain, ReadOnly, -15.0f);
//	RegisterFloatWithDefault(CameraDefaultTheta, ReadOnly, 20.0f);
//	//???DefineFloatWithDefault(CameraDefaultRho, 'CRHO', ReadWrite, n_deg2rad(10.0f));?
//END_ATTRS_REGISTRATION

namespace Scene
{
	class CSceneNode;
}

namespace Game
{
#define CameraMgr Game::CCameraManager::Instance()

class CCameraManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CCameraManager);

protected:

	bool					IsThirdPerson; // Cached to avoid RTTI checking. //???remove? Strict IsA() is fast
	Scene::CSceneNode*		pCameraNode;
	Scene::PNodeController	Ctlr;

	//nPFeedbackLoop<vector3>				Position;
	//nPFeedbackLoop<vector3>				Lookat;

	//collision
	//transition //???+ spline animation? - using node for COI of 3person camera is good for animated COI!
	//shaking

	//spline animation controller gets 4(?) points, factor from 0 to 1 and returns position on the spline
	//can use generic lerp animation with different lerps available

	//???here or in application and call methods of this manager?
	//DECLARE_EVENT_HANDLER(CameraReset, OnCameraReset);
	//DECLARE_EVENT_HANDLER(CameraOrbit, OnCameraOrbit);
	//DECLARE_EVENT_HANDLER(CameraDistance, OnCameraDistanceChange);

	//!!!OnActiveLevelChang(ing/ed)! - set controller from old to new camera, clear old camera modifiers like shake.
	//???what with camera anim? remember? and where, if yes?
	//!!!some data will be saved in the controller, if we will leave it on the old camera!

public:

	enum ETrackingMode
	{
		Track_None = 0,	// Target tracking disabled
		Track_Pos,		// Camera follows the target position
		Track_Chase		// Camera follows the target position and orientation
	};

	float	Sensitivity;
	float	MoveSpeed;
	float	ZoomSpeed;
	//???bool InvertY?

	CCameraManager(): pCameraNode(NULL), Sensitivity(0.05f), MoveSpeed(0.1f), ZoomSpeed(1.f) { __ConstructSingleton; }
	~CCameraManager() { __DestructSingleton; }

	bool					InitThirdPersonCamera(); //create controller (to attach to cameras later), attach if has active lvel //???setup aspect?
	void					ResetCamera(); //???reset only orientation (and distance for 3P)?

	void					SetTarget(const Scene::CSceneNode* pTarget); //!!!assert target is in the same scene as the camera!
	void					SetTargetTrackingMode(ETrackingMode Mode);

	void					EnableCollision(bool Enable);
	//void PreserveDistance(); // When object/terrain is under a camera, follow its surface

	Scene::CSceneNode*		GetCameraNode() const { return pCameraNode; }
	Scene::CNodeController*	GetCameraController() const { return Ctlr.GetUnsafe(); }
	bool					IsCameraThirdPerson() const { return IsThirdPerson; }

	//???force COI in parent space for 3P camera? default is zero vector
	//chasing is attaching camera to some entity
	//tracking is manual COI updating without touching an orientation
	//so, movement functions, like orientation and zooming ones, will be in a camera controller, not here
	//!!!FP camera moves itself, 3P camera moves COI!
};

typedef Ptr<CCameraManager> PCameraManager;

}

#endif
