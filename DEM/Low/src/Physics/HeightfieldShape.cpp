#include "HeightfieldShape.h"
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <Physics/BulletConv.h>

namespace Physics
{

CHeightfieldShape::CHeightfieldShape(btHeightfieldTerrainShape* pShape, PHeightfieldData&& HeightfieldData, const vector3& Offset)
	: CCollisionShape(pShape, Offset)
	, _HeightfieldData(std::move(HeightfieldData))
{
}
//---------------------------------------------------------------------

PCollisionShape CHeightfieldShape::CloneWithScaling(const vector3& Scaling) const
{
	/*
	const auto& CurrScaling = _pBtShape->getLocalScaling();
	const vector3 UnscaledOffset(_Offset.x / CurrScaling.x(), _Offset.y / CurrScaling.y(), _Offset.z / CurrScaling.z());
	//???scale offset?

	const auto pCurrBtShape = static_cast<btHeightfieldTerrainShape*>(_pBtShape);
	pCurrBtShape->setLocalScaling(VectorToBtVector(Scaling));

	PHeightfieldData _HeightfieldDataCopy;
	return n_new(CHeightfieldShape(new btHeightfieldTerrainShape(*pCurrBtShape), std::move(_HeightfieldDataCopy), UnscaledOffset));
	*/

	//???is needed?
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

}
