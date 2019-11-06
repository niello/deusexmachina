#include "CollisionAttribute.h"

#include <Scene/SceneNode.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CCollisionAttribute, Scene::CNodeAttribute);

Scene::PNodeAttribute CCollisionAttribute::Clone()
{
	//???or clone collision object? will CCollisionAttribute::Clone() ever be called?
	PCollisionAttribute ClonedAttr = n_new(CCollisionAttribute);
	ClonedAttr->CollObj = CollObj;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CCollisionAttribute::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);
	if (pNode->IsWorldMatrixChanged()) CollObj->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}
