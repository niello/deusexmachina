#include "PhysicsGfxUtil.h"

#include <Gfx/GfxServer.h>
#include <Gfx/ShapeEntity.h>
#include <Physics/Ragdoll.h>
#include <scene/ntransformnode.h>

namespace Graphics
{

void CPhysicsGfxUtil::CreateGraphics(Physics::CEntity* pPhysEntity, CGfxShapeArray& OutEntities)
{
	n_assert(pPhysEntity);
	CreateCompositeGraphics(pPhysEntity->GetComposite(), OutEntities);
}
//---------------------------------------------------------------------

bool CPhysicsGfxUtil::SetupGraphics(const nString& RsrcName, Physics::CEntity* pPhysEntity, const CGfxShapeArray& GfxEntities)
{
	n_assert(pPhysEntity);
	//??? FIXME: handle ragdolls based on pComp's type
	if (pPhysEntity->GetComposite()->IsA(Physics::CRagdoll::RTTI))
		return SetupRagdollGraphics(RsrcName, pPhysEntity, (Graphics::CCharEntity*) GfxEntities[0].get());
	else
		return SetupCompositeGraphics(RsrcName, pPhysEntity->GetComposite(), GfxEntities);
}
//---------------------------------------------------------------------

bool CPhysicsGfxUtil::UpdateGraphicsTransforms(Physics::CEntity* pPhysEntity, const CGfxShapeArray& GfxEntities)
{
	n_assert(pPhysEntity);
	if (pPhysEntity->GetComposite()->IsA(Physics::CRagdoll::RTTI))
		return TransferRagdollTransforms(pPhysEntity, (Graphics::CCharEntity*) GfxEntities[0].get());
	else
		return TransferCompositeTransforms(pPhysEntity->GetComposite(), GfxEntities);
}
//---------------------------------------------------------------------

void CPhysicsGfxUtil::CreateCompositeGraphics(Physics::CComposite* pComp, CGfxShapeArray& OutEntities)
{
	n_assert(pComp);

	bool Created = false;
	for (int i = 0; i < pComp->GetNumBodies(); i++)
	{
		Physics::CRigidBody* pBody = pComp->GetBodyAt(i);
		if (pBody->IsLinkValid(Physics::CRigidBody::ModelNode))
		{
			Created = true;
			pBody->LinkIndex = OutEntities.Size();
			OutEntities.Append(Graphics::CShapeEntity::Create());
		}
	}

	if (!Created) OutEntities.Append(Graphics::CShapeEntity::Create());
}
//---------------------------------------------------------------------

// Returns false if nothing has been done (because the pComp's rigid bodies have no ModelNode links
bool CPhysicsGfxUtil::SetupCompositeGraphics(const nString& RsrcName,
											Physics::CComposite* pComp,
											const CGfxShapeArray& GfxEntities)
{
	n_assert(RsrcName.IsValid());
	n_assert(pComp);
	n_assert(GfxEntities.Size() > 0);

	Graphics::CLevel* pGfxLevel = GfxSrv->GetLevel();
	n_assert(pGfxLevel);

	bool Created = false;
	for (int i = 0; i < pComp->GetNumBodies(); i++)
	{
		Physics::CRigidBody* pBody = pComp->GetBodyAt(i);
		if (pBody->IsLinkValid(Physics::CRigidBody::ModelNode))
		{
			Created = true;

			Graphics::CShapeEntity* pGfxEnt = GfxEntities[pBody->LinkIndex];
			pGfxEnt->SetTransform(pBody->GetTransform());

			nString BaseRsrcName = RsrcName;
			BaseRsrcName.TrimRight("/");
			BaseRsrcName.Append("/");
			pGfxEnt->SetResourceName(BaseRsrcName + pBody->GetLinkName(Physics::CRigidBody::ModelNode));

			if (pBody->IsLinkValid(Physics::CRigidBody::ShadowNode))
				pGfxEnt->SetShadowResourceName(BaseRsrcName + pBody->GetLinkName(Physics::CRigidBody::ShadowNode));

			pGfxLevel->AttachEntity(pGfxEnt);

			// Compute correctional matrices for the Nebula2 transform nodes, but only once per node!
			nTransformNode* pTfmNode = pGfxEnt->GetResource().GetNode();
			if (!pTfmNode->GetLocked())
			{
				pTfmNode->SetTransform(pTfmNode->GetTransform() * pBody->GetInvInitialTransform());

				if (pBody->IsLinkValid(Physics::CRigidBody::ShadowNode))
				{
					nTransformNode* pShdTfmNode = pGfxEnt->GetShadowResource().GetNode();
					if (!pShdTfmNode->GetLocked())
						pShdTfmNode->SetTransform(pShdTfmNode->GetTransform() * pBody->GetInvInitialTransform());
				}
			}
		}
	}

	if (!Created)
	{
		// Setup for simple graphics without physics
		n_assert(GfxEntities.Size() == 1);
		GfxEntities[0]->SetResourceName(RsrcName);
		GfxEntities[0]->SetTransform(pComp->GetTransform());
		pGfxLevel->AttachEntity(GfxEntities[0]);
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

// Transfer rigid body tfms to graphics entities. Returns false if nothing had been done.
bool CPhysicsGfxUtil::TransferCompositeTransforms(Physics::CComposite* pComp, const CGfxShapeArray& GfxEntities)
{
	n_assert(pComp);
	n_assert(GfxEntities.Size() > 0);

	bool Done = false;
	for (int i = 0; i < pComp->GetNumBodies(); i++)
	{
		Physics::CRigidBody* pBody = pComp->GetBodyAt(i);
		if (pBody->IsLinkValid(Physics::CRigidBody::ModelNode))
		{
			Done = true;
			GfxEntities[pBody->LinkIndex]->SetTransform(pBody->GetTransform());
		}
	}

	if (!Done)
	{
		for (int i = 0; i < GfxEntities.Size(); i++)
			GfxEntities[i]->SetTransform(pComp->GetTransform());
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CPhysicsGfxUtil::SetupRagdollGraphics(const nString& RsrcName,
										  Physics::CEntity* pPhysEnt,
										  Graphics::CCharEntity* pGfxEnt)
{
	//n_assert(pPhysEnt && pPhysEnt->GetComposite() && pPhysEnt->GetComposite()->IsA(Physics::CRagdoll::RTTI));
	//n_assert(pGfxEnt && pGfxEnt->IsA(Graphics::CCharEntity::RTTI));

	//pGfxEnt->SetTransform(pPhysEnt->GetTransform());
	//pGfxEnt->SetResourceName(RsrcName);
	//GfxSrv->GetLevel()->AttachEntity(pGfxEnt);

	//Physics::CRagdoll* pRagdoll = (Physics::CRagdoll*)pPhysEnt->GetComposite();
	//pRagdoll->SetCharacter(pGfxEnt->GetCharacterPointer());
	//pRagdoll->Bind();

	OK;
}
//---------------------------------------------------------------------

bool CPhysicsGfxUtil::TransferRagdollTransforms(Physics::CEntity* pPhysEnt, Graphics::CCharEntity* pGfxEnt)
{
	//n_assert(pPhysEnt && pPhysEnt->GetComposite() && pPhysEnt->GetComposite()->IsA(Physics::CRagdoll::RTTI));
	//n_assert(pGfxEnt && pGfxEnt->IsA(Graphics::CCharEntity::RTTI));

	//pGfxEnt->SetTransform(pPhysEnt->GetTransform());

	//Physics::CRagdoll* pRagdoll = (Physics::CRagdoll*)pPhysEnt->GetComposite();
	//pRagdoll->WriteJoints();

	OK;
}
//---------------------------------------------------------------------

} // namespace Util
