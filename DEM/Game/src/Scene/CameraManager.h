#pragma once
#ifndef __DEM_L2_CAMERA_MANAGER_H__
#define __DEM_L2_CAMERA_MANAGER_H__

//#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Scene/NodeControllerThirdPerson.h>
//#include <Frame/CameraAttribute.h>
//#include <Util/PFLoop.h>

// Manages the current camera of the game level. Provides interface for driving camera
// (manually, by input, by script), performing smooth transition between changing targets,
// chasing or tracking some scene node, shaking to simulate explosions and quakes etc.
// Camera manager also takes care of camera collisions and target size dependent limiting.

//!!!L2 CameraManager:
// Third-person COI rotation with 2-sided vertical angle limitimg
// COI moving
// COI from scene node (chase/track)
// Smooth move from one to another target, changing COI and position
// Control by UI and API
// Set camera (which camera to use)
// Focus on entity - smooth move from curr state to new object

namespace Scene
{
class CSceneNode;

class CCameraManager: public Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	bool			IsThirdPerson; //???need? // Cached to avoid RTTI checking
	CSceneNode*		pCameraNode;
	PNodeController	Ctlr;

	//CPFLoop<vector3>				Position;
	//CPFLoop<vector3>				Lookat;

	//accelerations, if smoothing needed

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
	//???RotationSpeed; , or it is a sensitivity?
	//???bool InvertY?

	CCameraManager(): pCameraNode(nullptr), Sensitivity(0.05f), MoveSpeed(0.1f), ZoomSpeed(1.f) { }

	bool				InitThirdPersonCamera(CSceneNode& NodeWithCamera);
	void				ResetCamera(); //???reset only orientation (and distance for 3P)?

	void				SetTarget(const CSceneNode* pTarget, const vector3& Offset = vector3::Zero); //!!!assert target is in the same scene as the camera!
	void				SetTargetTrackingMode(ETrackingMode Mode);

	void				EnableCollision(bool Enable);
	//void PreserveDistance(); // When object/terrain is under a camera, follow its surface

	CSceneNode*			GetCameraNode() const { return pCameraNode; }
	CNodeController*	GetCameraController() const { return Ctlr.Get(); }
	bool				IsCameraThirdPerson() const { return IsThirdPerson; }

	//???force COI in parent space for 3P camera? default is zero vector
	//chasing is attaching camera to some entity
	//tracking is manual COI updating without touching an orientation
	//so, movement functions, like orientation and zooming ones, will be in a camera controller, not here
	//!!!FP camera moves itself, 3P camera moves COI!
};

typedef Ptr<CCameraManager> PCameraManager;

}

#endif
