#include "CollisionAttribute.h"
#include <Physics/CollisionObjMoving.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Physics
{
FACTORY_CLASS_IMPL(Physics::CCollisionAttribute, 'COLA', Scene::CNodeAttribute);

bool CCollisionAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'SHAP':
			{
				ShapeUID = DataReader.Read<CStrID>();
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CCollisionAttribute::Clone()
{
	PCollisionAttribute ClonedAttr = n_new(CCollisionAttribute());
	ClonedAttr->ShapeUID = ShapeUID;
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CCollisionAttribute::Update(const vector3* pCOIArray, UPTR COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);
	if (Collider && pNode->IsWorldMatrixChanged()) Collider->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

bool CCollisionAttribute::ValidateResources(Resources::CResourceManager& ResMgr, CPhysicsLevel& Level)
{
	Resources::PResource RShape = ResMgr.RegisterResource<Physics::CCollisionShape>(ShapeUID);
	if (!RShape) FAIL;

	auto Shape = RShape->ValidateObject<Physics::CCollisionShape>();
	if (!Shape) FAIL;

	// create collision object using shape, initial tfm, group, mask and probably friction & restitution
	//Collider = Level.CreateMovableCollider(Shape);
	//store level inside a collider to self-remove it on destroy

	OK;
}
//---------------------------------------------------------------------

bool CCollisionAttribute::GetGlobalAABB(CAABB& OutBox) const
{
	if (!Collider) FAIL;
	Collider->GetGlobalAABB(OutBox);
	OK;
}
//---------------------------------------------------------------------

}
