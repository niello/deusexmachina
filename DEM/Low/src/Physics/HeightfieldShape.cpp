#include "HeightfieldShape.h"
#include <Physics/BulletConv.h>

namespace Physics
{

CHeightfieldShape::CHeightfieldShape(btDEMHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const rtm::vector4f& Offset, const rtm::vector4f& Scaling)
	: CCollisionShape(pShape, Offset, Scaling)
	, _HeightfieldData(std::move(HeightfieldData))
{
	// Accelerate raycasting
	pShape->buildAccelerator();
}
//---------------------------------------------------------------------

// FIXME: terrain is almost never shared, could scale in place instead of calling this! Need to fix in design!
PCollisionShape CHeightfieldShape::CloneWithScaling(const rtm::vector4f& Scaling) const
{
	const rtm::vector4f UnscaledOffset = rtm::vector_div(_Offset, Math::FromBullet(_pBtShape->getLocalScaling()));

	auto pBtHFShape = static_cast<btDEMHeightfieldTerrainShape*>(_pBtShape);

	const UPTR DataSize = pBtHFShape->GetHeightfieldWidth() * pBtHFShape->GetHeightfieldHeight() * sizeof(short);
	PHeightfieldData NewData(new char[DataSize]);
	std::memcpy(NewData.get(), _HeightfieldData.get(), DataSize);

	auto pNewBtShape = new btDEMHeightfieldTerrainShape(
		pBtHFShape->GetHeightfieldWidth(),
		pBtHFShape->GetHeightfieldHeight(),
		NewData.get(),
		pBtHFShape->GetHeightScale(),
		pBtHFShape->GetMinHeight(),
		pBtHFShape->GetMaxHeight(),
		1,
		PHY_SHORT,
		false);

	return new Physics::CHeightfieldShape(pNewBtShape, std::move(NewData), UnscaledOffset, Scaling);
}
//---------------------------------------------------------------------

}
