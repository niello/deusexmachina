#include "CharacterController.h"

#include <Physics/PhysicsServer.h>
#include <Physics/RigidBody.h>
#include <Data/Params.h>

namespace Physics
{
//__ImplementClass(Physics::CCharacterController, 'CCTL', Core::CRefCounted);

bool CCharacterController::Init(const Data::CParams& Desc)
{
	Radius = Desc.Get<float>(CStrID("Radius"), 0.3f);
	Height = Desc.Get<float>(CStrID("Height"), 1.75f);
	Hover = Desc.Get<float>(CStrID("Hover"), 0.2f);
	float Mass = Desc.Get<float>(CStrID("Mass"), 80.f);

	float CapsuleHeight = Height - Radius - Radius - Hover;
	n_assert (CapsuleHeight > 0.f);

	PCollisionShape Shape = PhysicsSrv->CreateCapsuleShape(Radius, CapsuleHeight);

	vector3 Offset(0.f, Hover, 0.f); //???!!! + Radius + Height * 0.5f ?!

	Data::PParams BodyDesc = n_new(Data::CParams);
	BodyDesc->Set(CStrID("Shape"), Shape->GetUID());
	BodyDesc->Set(CStrID("Group"), nString("Character"));
	BodyDesc->Set(CStrID("Mask"), nString("All"));
	BodyDesc->Set(CStrID("Mass"), Mass);

	Body = n_new(CRigidBody);
	Body->Init(*BodyDesc, Offset); //!!!!!!!need another Init with explicit params and Shape ptr, not ID. Nonvirtual.

	OK;
}
//---------------------------------------------------------------------

}
