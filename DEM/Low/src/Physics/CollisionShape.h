#pragma once
#include <Core/Object.h>
#include <Math/Vector3.h>
#include <rtm/vector4f.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{
using PCollisionShape = Ptr<class CCollisionShape>;

class CCollisionShape : public ::Core::CObject
{
	RTTI_CLASS_DECL(Physics::CCollisionShape, ::Core::CObject);

protected:

	btCollisionShape* _pBtShape = nullptr;
	rtm::vector4f     _Offset; // Could use btCompoundShape instead, but for offset only it is an overkill

public:

	static PCollisionShape CreateSphere(float Radius, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	static PCollisionShape CreateBox(const rtm::vector4f& Size, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	static PCollisionShape CreateCapsuleX(float Radius, float CylinderLength, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	static PCollisionShape CreateCapsuleY(float Radius, float CylinderLength, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	static PCollisionShape CreateCapsuleZ(float Radius, float CylinderLength, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	static PCollisionShape CreateConvexHull(const vector3* pVertices, UPTR VertexCount, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	// TODO: CreateMesh(Render::PMeshData MeshData)

	CCollisionShape(btCollisionShape* pBtShape, const rtm::vector4f& Offset = rtm::vector_zero(), const rtm::vector4f& Scaling = rtm::vector_set(1.f));
	virtual ~CCollisionShape() override;

	virtual PCollisionShape CloneWithScaling(const rtm::vector4f& Scaling) const;
	const rtm::vector4f&    GetOffset() const { return _Offset; }
	btCollisionShape*       GetBulletShape() const { return _pBtShape; }
};

}
