#include "HeightfieldShape.h"
#include <Physics/BulletConv.h>

namespace Physics
{

CHeightfieldShape::CHeightfieldShape(btDEMHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset)
	: CCollisionShape(pShape, Offset)
	, _HeightfieldData(std::move(HeightfieldData))
{
}
//---------------------------------------------------------------------

// FIXME: terrain is almost never shared, could scale in place instead of calling this! Need to fix in design!
PCollisionShape CHeightfieldShape::CloneWithScaling(const vector3& Scaling) const
{
	const auto& CurrScaling = _pBtShape->getLocalScaling();
	const vector3 Rescale(Scaling.x / CurrScaling.x(), Scaling.y / CurrScaling.y(), Scaling.z / CurrScaling.z());

	auto pBtHFShape = static_cast<btDEMHeightfieldTerrainShape*>(_pBtShape);

	const UPTR DataSize = pBtHFShape->GetHeightfieldWidth() * pBtHFShape->GetHeightfieldHeight() * sizeof(short);
	PHeightfieldData NewData(new char[DataSize]);
	std::memcpy(NewData.get(), _HeightfieldData.get(), DataSize);

	auto pNewBtShape = new btDEMHeightfieldTerrainShape(
		pBtHFShape->GetHeightfieldWidth(),
		pBtHFShape->GetHeightfieldHeight(),
		NewData.get(),
		pBtHFShape->GetHeightScale(),
		pBtHFShape->GetMinHeight() * Rescale.y,
		pBtHFShape->GetMaxHeight() * Rescale.y,
		1,
		PHY_SHORT,
		false);
	pNewBtShape->setLocalScaling(VectorToBtVector(Scaling));
	pNewBtShape->buildAccelerator();

	const vector3 NewOffset(_Offset.x * Rescale.x, _Offset.y * Rescale.y, _Offset.z * Rescale.z);
	return new Physics::CHeightfieldShape(pNewBtShape, std::move(NewData), NewOffset);
}
//---------------------------------------------------------------------

}
