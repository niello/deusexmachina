#pragma once
#include <Physics/CollisionShape.h>
#include <Math/Vector3.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// Heightfield collision shape, which owns heightmap data (and possibly can modify it)

// TODO: add getters to Bullet via PR instead
class btDEMHeightfieldTerrainShape : public btHeightfieldTerrainShape
{
public:

	using btHeightfieldTerrainShape::btHeightfieldTerrainShape;

	int      GetHeightfieldWidth() const { return m_heightStickWidth; }
	int      GetHeightfieldHeight() const { return m_heightStickLength; }
	btScalar GetMinHeight() const { return m_minHeight; }
	btScalar GetMaxHeight() const { return m_maxHeight; }
	btScalar GetHeightScale() const { return m_heightScale; }
};

namespace Physics
{
typedef std::unique_ptr<char[]> PHeightfieldData;

class CHeightfieldShape : public CCollisionShape
{
	RTTI_CLASS_DECL(Physics::CHeightfieldShape, Physics::CCollisionShape);

protected:

	PHeightfieldData _HeightfieldData;

public:

	CHeightfieldShape(btDEMHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset = vector3::Zero);

	virtual PCollisionShape CloneWithScaling(const vector3& Scaling) const override;
};

typedef Ptr<CHeightfieldShape> PHeightfieldShape;

}
