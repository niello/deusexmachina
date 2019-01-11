#include "NodeAttrCollision.h"

#include <Scene/SceneNode.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CNodeAttrCollision, Scene::CNodeAttribute);

Scene::PNodeAttribute CNodeAttrCollision::Clone()
{
	//???or clone collision object? will CNodeAttrCollision::Clone() ever be called?
	PNodeAttrCollision ClonedAttr = n_new(CNodeAttrCollision);
	ClonedAttr->CollObj = CollObj;
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CNodeAttrCollision::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);
	if (pNode->IsWorldMatrixChanged()) CollObj->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}
