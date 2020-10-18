#pragma once
#include <Resources/ResourceObject.h>
#include <Math/Vector3.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{
using PCollisionShape = Ptr<class CCollisionShape>;

class CCollisionShape : public Resources::CResourceObject
{
	RTTI_CLASS_DECL(Physics::CCollisionShape, Resources::CResourceObject);

protected:

	btCollisionShape* _pBtShape = nullptr;
	vector3           _Offset; // Could use btCompoundShape instead, but for offset only it is an overkill

public:

	static PCollisionShape CreateSphere(float Radius, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	static PCollisionShape CreateBox(const vector3& Size, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	static PCollisionShape CreateCapsuleX(float Radius, float CylinderLength, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	static PCollisionShape CreateCapsuleY(float Radius, float CylinderLength, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	static PCollisionShape CreateCapsuleZ(float Radius, float CylinderLength, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	// TODO: CreateMesh(Render::PMeshData MeshData)

	CCollisionShape(btCollisionShape* pBtShape, const vector3& Offset = vector3::Zero, const vector3& Scaling = vector3::One);
	virtual ~CCollisionShape() override;

	virtual bool            IsResourceValid() const { return !!_pBtShape; }
	virtual PCollisionShape CloneWithScaling(const vector3& Scaling) const;
	const vector3&          GetOffset() const { return _Offset; }
	btCollisionShape*       GetBulletShape() const { return _pBtShape; }
};

}
