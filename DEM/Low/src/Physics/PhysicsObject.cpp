#include "PhysicsObject.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/HeightfieldShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace Physics
{

CPhysicsObject::CPhysicsObject(CStrID CollisionGroupID, CStrID CollisionMaskID)
	: _CollisionGroupID(CollisionGroupID)
	, _CollisionMaskID(CollisionMaskID)
{
}
//---------------------------------------------------------------------

// NB: treat this as a part of constructor, each child must call it from its constructor
void CPhysicsObject::ConstructInternal(btCollisionObject* pBtObject, const CPhysicsMaterial& Material)
{
	n_assert_dbg(pBtObject);

	_pBtObject = pBtObject;

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	// Can't guarantee that pShape is not (or will not be) shared
	_IsShapeShared = true;

	// Instead of storing strong ref, we manually control refcount and use
	// a pointer from the bullet collision shape
	pShape->AddRef();

	// As of Bullet v2.89 SDK, debug drawer tries to draw each heightfield triangle wireframe,
	// so we disable debug drawing of terrain at all
	// TODO: terrain is most probably a static collider, not movable!
	if (pShape->IsA<CHeightfieldShape>())
		_pBtObject->setCollisionFlags(_pBtObject->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);

	_pBtObject->setFriction(Material.Friction);
	_pBtObject->setRollingFriction(Material.RollingFriction);
	_pBtObject->setRestitution(1.f - Material.Bounciness);
	_pBtObject->setUserPointer(this);
}
//---------------------------------------------------------------------

CPhysicsObject::~CPhysicsObject()
{
	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
	delete _pBtObject;
	pShape->Release(); // See ConstructInternal
}
//---------------------------------------------------------------------

// Bullet Physics doesn't support per-instance scaling of shared shapes. When scale changes, we must create our
// personal shape copy and scale it as we want. Obviously unsharing must be done only once.
bool CPhysicsObject::UnshareShapeIfNecessary(const rtm::vector4f& NewScaling)
{
	if (!_IsShapeShared) return true;

	constexpr float ShapeUnshareThreshold = 0.0001f;

	const btVector3& ShapeScaling = _pBtObject->getCollisionShape()->getLocalScaling();
	if (!n_fequal(NewScaling.x, ShapeScaling.x(), ShapeUnshareThreshold) ||
		!n_fequal(NewScaling.y, ShapeScaling.y(), ShapeUnshareThreshold) ||
		!n_fequal(NewScaling.z, ShapeScaling.z(), ShapeUnshareThreshold))
	{
		auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());
		PCollisionShape NewShape = pShape->CloneWithScaling(NewScaling);
		if (!NewShape) return false;

		_pBtObject->setCollisionShape(NewShape->GetBulletShape());
		_IsShapeShared = false;

		// See ConstructInternal for explanation
		pShape->Release();
		NewShape->AddRef();
	}

	return true;
}
//---------------------------------------------------------------------

bool CPhysicsObject::PrepareTransform(const rtm::matrix3x4f& NewTfm, btTransform& OutTfm)
{
	const rtm::vector4f Scaling = Math::matrix_extract_scale(NewTfm);

	if (!UnshareShapeIfNecessary(Scaling)) return false;

	auto pShape = static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer());

	const rtm::vector4f AxisX = rtm::vector_div(NewTfm.x_axis, rtm::vector_dup_x(Scaling));
	const rtm::vector4f AxisY = rtm::vector_div(NewTfm.y_axis, rtm::vector_dup_y(Scaling));
	const rtm::vector4f AxisZ = rtm::vector_div(NewTfm.z_axis, rtm::vector_dup_z(Scaling));

	OutTfm.setBasis(
		btMatrix3x3(
			rtm::vector_get_x(AxisX), rtm::vector_get_x(AxisY), rtm::vector_get_x(AxisZ),
			rtm::vector_get_y(AxisX), rtm::vector_get_y(AxisY), rtm::vector_get_y(AxisZ),
			rtm::vector_get_z(AxisX), rtm::vector_get_z(AxisY), rtm::vector_get_z(AxisZ)));

	//???TODO PERF: optimize origin? VectorToBtVector(Offset).dot3(m_basis[0], m_basis[1], m_basis[2]) + VectorToBtVector(Tfm.w_axis)
	OutTfm.setOrigin(VectorToBtVector(NewTfm.w_axis));
	OutTfm.getOrigin() = OutTfm * VectorToBtVector(pShape->GetOffset());

	return true;
}
//---------------------------------------------------------------------

void CPhysicsObject::AttachToLevel(CPhysicsLevel& Level)
{
	if (_Level == &Level) return;

	if (_Level) RemoveFromLevel();

	_Level = &Level;
	AttachToLevelInternal();
}
//---------------------------------------------------------------------

void CPhysicsObject::RemoveFromLevel()
{
	if (!_Level) return;
	RemoveFromLevelInternal();
	_Level = nullptr;
}
//---------------------------------------------------------------------

// Returns end-of-tick AABB from the physics world
void CPhysicsObject::GetPhysicsAABB(Math::CAABB& OutBox) const
{
	if (!_pBtObject) return;

	btVector3 Min, Max;
	if (_Level)
		_Level->GetBtWorld()->getBroadphase()->getAabb(_pBtObject->getBroadphaseHandle(), Min, Max);
	else
		_pBtObject->getCollisionShape()->getAabb(_pBtObject->getWorldTransform(), Min, Max);
	OutBox.Min = BtVectorToVector(Min);
	OutBox.Max = BtVectorToVector(Max);
}
//---------------------------------------------------------------------

const CCollisionShape* CPhysicsObject::GetCollisionShape() const
{
	return _pBtObject ? static_cast<CCollisionShape*>(_pBtObject->getCollisionShape()->getUserPointer()) : nullptr;
}
//---------------------------------------------------------------------

bool CPhysicsObject::IsActive() const
{
	return _pBtObject ? _pBtObject->isActive() : false;
}
//---------------------------------------------------------------------

bool CPhysicsObject::IsAlwaysActive() const
{
	return _pBtObject ? (_pBtObject->getActivationState() == DISABLE_DEACTIVATION) : false;
}
//---------------------------------------------------------------------

}
