#include "NodeControllerThirdPerson.h"

namespace Scene
{

//bool CThirdPersonCameraController::ApplyTo(Math::CTransformSRT& DestTfm)
//{
//	if (!Dirty) FAIL;
//
//	quaternion Qx, Qy;
//	Qx.set_rotate_x(-Angles.Theta);
//	Qy.set_rotate_y(-Angles.Phi);
//
//	// Optimized Qx * Qy. Z is negated to switch handedness to RH.
//	DestTfm.Rotation.set(Qx.x * Qy.w, Qx.w * Qy.y, -Qx.x * Qy.y, Qx.w * Qy.w);
//
//	DestTfm.Translation = COI + Angles.GetCartesianZ() * Distance;
//
//	Dirty = false;
//	OK;
//}
////---------------------------------------------------------------------

}
