#include "AnimControllerThirdPerson.h"

namespace Scene
{

bool CAnimControllerThirdPerson::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Dirty) FAIL;

	//!!!it fails if substituted below! rewrite quat mul!
	//DestTfm.Rotation.set_rotate_x(-Angles.theta);
	//DestTfm.Rotation = DestTfm.Rotation * Qy;

	quaternion Qx, Qy;
	Qx.set_rotate_x(-Angles.theta);
	Qy.set_rotate_y(-Angles.phi);
	DestTfm.Rotation = Qx * Qy;
	DestTfm.Rotation.z *= -1.f; // Switch handedness to RH

	DestTfm.Translation = Angles.get_cartesian_z() * Distance;

	Dirty = false;
	OK;
}
//---------------------------------------------------------------------

}
