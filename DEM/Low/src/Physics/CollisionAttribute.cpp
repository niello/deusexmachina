#include "CollisionAttribute.h"
#include <Physics/StaticCollider.h>
#include <Physics/MovableCollider.h>
#include <Physics/CollisionShape.h>
#include <Physics/PhysicsLevel.h>
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
			case 'STAT':
			{
				Static = DataReader.Read<bool>();
				break;
			}
			case 'COGR':
			{
				CollisionGroupID = DataReader.Read<CStrID>();
				break;
			}
			case 'COMA':
			{
				CollisionMaskID = DataReader.Read<CStrID>();
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
	ClonedAttr->CollisionGroupID = CollisionGroupID;
	ClonedAttr->CollisionMaskID = CollisionMaskID;
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CCollisionAttribute::UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount)
{
	if (!Collider) return;

	if (pNode->GetTransformVersion() != LastTransformVersion)
	{
		Collider->SetTransform(pNode->GetWorldMatrix());
		LastTransformVersion = pNode->GetTransformVersion();
	}
	else
	{
		Collider->SetActive(false);
	}
}
//---------------------------------------------------------------------

bool CCollisionAttribute::ValidateResources(Resources::CResourceManager& ResMgr, CPhysicsLevel& Level)
{
	if (!pNode) FAIL;

	Resources::PResource RShape = ResMgr.RegisterResource<Physics::CCollisionShape>(ShapeUID);
	if (!RShape) FAIL;

	auto Shape = RShape->ValidateObject<Physics::CCollisionShape>();
	if (!Shape) FAIL;

	const U16 Group = Level.CollisionGroups.GetMask(CollisionGroupID ? CollisionGroupID.CStr() : "Default");
	const U16 Mask = Level.CollisionGroups.GetMask(CollisionMaskID ? CollisionMaskID.CStr() : "All");

	// Also can create bullet kinematic body right here without CMovableCollider wrapper, think of it
	if (Static)
		Collider = n_new(Physics::CStaticCollider(Level, *Shape, Group, Mask, pNode->GetWorldMatrix()));
	else
		Collider = n_new(Physics::CMovableCollider(Level, *Shape, Group, Mask, pNode->GetWorldMatrix()));

	// Just updated, save redundant update
	LastTransformVersion = pNode->GetTransformVersion();

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
