#include "AnimControllerThirdPerson.h"

namespace Scene
{

bool CAnimControllerThirdPerson::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Dirty) FAIL;

	quaternion Qx, Qy;
	Qx.set_rotate_x(-Angles.theta);
	Qy.set_rotate_y(-Angles.phi);
	//DestTfm.Rotation = Qx * Qy;
	//DestTfm.Rotation.z *= -1.f; // Switch handedness to RH

	// Optimized Qx * Qy. Z is negated to switch handedness to RH.
	DestTfm.Rotation.set(Qx.x * Qy.w, Qx.w * Qy.y, -Qx.x * Qy.y, Qx.w * Qy.w);

	DestTfm.Translation = COI + Angles.get_cartesian_z() * Distance;

	Dirty = false;
	OK;
}
//---------------------------------------------------------------------

}
