#include "Shape.h"

#include <Physics/RigidBody.h>
#include <Physics/FilterSet.h>
#include <Physics/ContactPoint.h>
#include <Physics/PhysicsLevel.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CShape, Core::CRefCounted);

nArray<CContactPoint>* CShape::CollideContacts = NULL;
const CFilterSet* CShape::CollideFilterSet = NULL;

CShape::CShape(EType ShapeType):
	Type(ShapeType),
	pRigidBody(NULL),
	pEntity(NULL),
	CollCount(0),
	IsHorizColl(false),
	CatBits(Dynamic),
	CollBits(All),
	ODEGeomID(NULL),
	ODESpaceID(NULL),
	MaterialType(InvalidMaterial)
{
	//MaterialType = CMaterialTable::StringToMaterialType("Metal"); //???invalid material?
	dMassSetZero(&ODEMass); //!!!need to calculate mass somewhere!
}
//---------------------------------------------------------------------

CShape::~CShape()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

// This attaches the pShape to a new collide space. This method is more
// lightweight then Attach() (in fact, it's called by Attach(). This method
// together with RemoveFromSpace() can be used to move the pShape
// quickly between collide spaces.
void CShape::AttachToSpace(dSpaceID SpaceID)
{
	n_assert(!ODESpaceID);
	n_assert(ODEGeomID);
	ODESpaceID = SpaceID;
	dSpaceAdd(SpaceID, ODEGeomID);
}
//---------------------------------------------------------------------

// This removes the pShape from its current collide space, but leaves everything intact.
void CShape::RemoveFromSpace()
{
	n_assert(ODESpaceID);
	n_assert(ODEGeomID);
	dSpaceRemove(ODESpaceID, ODEGeomID);
	ODESpaceID = NULL;
}
//---------------------------------------------------------------------

//???Overridden in subclasses?
bool CShape::Attach(dSpaceID SpaceID)
{
    n_assert(!IsAttached());
    OK;
}
//---------------------------------------------------------------------

void CShape::Detach()
{
	n_assert(IsAttached());
	n_assert(ODEGeomID);
	dGeomDestroy(ODEGeomID);
	ODEGeomID = NULL;
	ODESpaceID = NULL;
}
//---------------------------------------------------------------------

// Returns true if collision is valid
bool CShape::OnCollide(CShape* pShape)
{
	n_assert(pShape);
	if (pEntity && pShape->pEntity != pEntity)
		return pEntity->OnCollide(pShape);
	OK;
}
//---------------------------------------------------------------------

// Universal method for all specific ODE geom types, which add the
// geom to the collide space, using an ODE proxy geom to offset the
// geom by the provided transformation matrix. The geom will also
// be attached to the rigid body, if any is set.
void CShape::AttachGeom(dGeomID GeomId, dSpaceID SpaceID)
{
	n_assert(GeomId);
	n_assert(!IsAttached());

	// set the geom's local Transform
	const vector3& Pos = Transform.Translation();
	dGeomSetPosition(GeomId, Pos.x, Pos.y, Pos.z);
	dMatrix3 ODERotation;
	CPhysicsServer::Matrix44ToOde(Transform, ODERotation);
	dGeomSetRotation(GeomId, ODERotation);

	// if attached to rigid body, create a geom Transform "proxy" object && attach it to the rigid body
	// else directly set Transform and rotation
	if (pRigidBody)
	{
		ODEGeomID = dCreateGeomTransform(0);
		dGeomTransformSetCleanup(ODEGeomID, 1);
		dGeomTransformSetGeom(ODEGeomID, GeomId);
		dGeomSetBody(ODEGeomID, pRigidBody->GetODEBodyID());
	}
	else ODEGeomID = GeomId;

	dGeomSetCategoryBits(ODEGeomID, CatBits);
	dGeomSetCollideBits(ODEGeomID, CollBits);
	dGeomSetData(ODEGeomID, this);
	AttachToSpace(SpaceID);
}
//---------------------------------------------------------------------

// Transform the own dMass structure by the own Transform matrix.
void CShape::TransformMass()
{
	dMatrix3 ODERotation;
	CPhysicsServer::Matrix44ToOde(Transform, ODERotation);
	dMassRotate(&ODEMass, ODERotation);
	const vector3& Pos = Transform.Translation();
	dMassTranslate(&ODEMass, Pos.x, Pos.y, Pos.z);
}
//---------------------------------------------------------------------

void CShape::GetAABB(bbox3& AABB) const
{
	if (ODEGeomID)
	{
		dReal Box[6];
		dGeomGetAABB(ODEGeomID, Box);
		AABB.vmin.x = Box[0];
		AABB.vmax.x = Box[1];
		AABB.vmin.y = Box[2];
		AABB.vmax.y = Box[3];
		AABB.vmin.z = Box[4];
		AABB.vmax.z = Box[5];
		//???transform?
	}
	else
	{
		AABB.vmin.x = 
		AABB.vmin.y = 
		AABB.vmin.z = 
		AABB.vmax.x = 
		AABB.vmax.y = 
		AABB.vmax.z = 0.f;
	}
}
//---------------------------------------------------------------------

void CShape::SetTransform(const matrix44& Tfm)
{
	Transform = Tfm;

	// if not attached to rigid body, directly update pShape's transformation
	if (!pRigidBody && ODEGeomID)
	{
		//!!!DUPLICATE CODE!
		const vector3& Pos = Transform.Translation();
		dGeomSetPosition(ODEGeomID, Pos.x, Pos.y, Pos.z);
		dMatrix3 ODERotation;
		CPhysicsServer::Matrix44ToOde(Transform, ODERotation);
		dGeomSetRotation(ODEGeomID, ODERotation);
	}
}
//---------------------------------------------------------------------

// Returns the debug visualization color which is used in RenderDebug().
// This depends on the state of the rigid body which owns this state (if any):
// yellow:  no rigid body attached to pShape
// green:   rigid body is enabled
// blue:    ridig body is disabled
vector4 CShape::GetDebugVisualizationColor() const
{
	return (pRigidBody) ?
				((pRigidBody->IsEnabled()) ?
					vector4(0.0f, 1.0f, 0.0f, 0.75f) :
					vector4(0.0f, 0.0f, 1.0f, 0.75f)) :
				vector4(1.0f, 1.0f, 0.0f, 0.3f);
}
//---------------------------------------------------------------------

// The ODENearCallback for CShape::Collide.
//
// 31-May-05   floh    invert contact normal if necessary, the contact normal
//                     in ODE always points into pShape1, however, we want
//                     the contact normal always to point away from the
//                     "other" object
void CShape::ODENearCallback(void* data, dGeomID o1, dGeomID o2)
{
	CShape* pThis = (CShape*)data;

	// collide geom with sub-space
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2(o1, o2, data, &ODENearCallback);
		return;
	}

	CShape* pShape1 = CShape::GetShapeFromGeom(o1);
	CShape* pShape2 = CShape::GetShapeFromGeom(o2);
	CShape* pOtherShape;
	if (pShape1 == pThis) pOtherShape = pShape2;
	else if (pShape2 == pThis) pOtherShape = pShape1;
	else n_error("No self in collision");

	if (CShape::CollideFilterSet->CheckShape(pOtherShape)) return;

	// do collision detection, only check for one contact
	dContactGeom ODEContact;
	int CurrCollCount;
	if (pShape1 == pThis) CurrCollCount = dCollide(o1, o2, 1, &ODEContact, sizeof(dContactGeom));
	else CurrCollCount = dCollide(o2, o1, 1, &ODEContact, sizeof(dContactGeom));

	CContactPoint ContactPt;
	if (CurrCollCount > 0)
	{
		ContactPt.Position.set(ODEContact.pos[0], ODEContact.pos[1], ODEContact.pos[2]);
		ContactPt.UpVector.set(ODEContact.normal[0], ODEContact.normal[1], ODEContact.normal[2]);
		ContactPt.Material = pOtherShape->GetMaterialType();
		ContactPt.Depth = ODEContact.depth;
		CEntity* pEntity = pOtherShape->GetEntity();
		if (pEntity) ContactPt.EntityID = pEntity->GetUID();
		CRigidBody* pRigidBody = pOtherShape->GetRigidBody();
		if (pRigidBody) ContactPt.RigidBodyID = pRigidBody->GetUID();
		CShape::CollideContacts->Append(ContactPt);
	}
}
//---------------------------------------------------------------------

// Collides this pShape against the rest of the world and return all found
// contacts. Note this method will not return exact contacts, since there
// will only be one contact point returned per collided geom. Contact positions
// will be set to the midpoint of the geom, normals will be computed so that
// they point away from the CShape's midpoint.
void CShape::Collide(const CFilterSet& FilterSet, nArray<CContactPoint>& Contacts)
{
	Contacts.Reset();
	CShape::CollideContacts = &Contacts;
	CShape::CollideFilterSet = &FilterSet;
	//dSpaceCollide2((dGeomID)PhysicsSrv->GetLevel()->GetODECommonSpaceID(), ODEGeomID, this, &ODENearCallback);
}
//---------------------------------------------------------------------

} // namespace Physics
