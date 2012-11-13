#include "Entity.h"

#include <Gfx/GfxServer.h>
#include <Gfx/CameraEntity.h>
#include <mathlib/sphere.h>
#include <gfx2/ngfxserver2.h>

namespace Graphics
{
ImplementRTTI(Graphics::CEntity, Core::CRefCounted);

CEntity::CEntity():
	Flags(VISIBLE),
	pLevel(NULL),
	pQTNode(NULL),
	Links(NumLinkTypes),
	ActivationTime(0.0),
	TimeFactor(1.0f),
	MaxVisibleDistance(10000.0f),
	MinVisibleSize(0.0f),
	UserData(CStrID::Empty)
{
	// normal entities need rather small arrays, while cameras and
	// lights may need bigger arrays...
	for (int LinkType = 0; LinkType < NumLinkTypes; LinkType++)
		Links[LinkType].SetFlags(nArray<PEntity>::DoubleGrowSize);
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	n_assert(!pQTNode);
	if (Flags.Is(ACTIVE)) Deactivate();
}
//---------------------------------------------------------------------

// This method is called when the graphics object is attached to a game entity.
void CEntity::Activate()
{
	n_assert(!Flags.Is(ACTIVE));
	ResetActivationTime();
	Flags.Set(ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(Flags.Is(ACTIVE));

	for (int LinkType = 0; LinkType < NumLinkTypes; LinkType++)
		ClearLinks((ELinkType)LinkType);

	Flags.Clear(ACTIVE);
}
//---------------------------------------------------------------------

void CEntity::RenderDebug()
{
	nGfxServer2::Instance()->DrawShape(nGfxServer2::Box, GlobalBBox.to_matrix44(), pQTNode->Data.DebugColor);
}
//---------------------------------------------------------------------

void CEntity::SetTransform(const matrix44& Tfm)
{
	Transform = Tfm;
	Flags.Set(GLOBAL_BOX_DIRTY);
	if (pLevel && pQTNode) pLevel->UpdateEntityLocation(this);
}
//---------------------------------------------------------------------

void CEntity::ResetActivationTime()
{
	ActivationTime = ExtTimeSrc.isvalid() ? ExtTimeSrc->GetTime() : GfxSrv->EntityTimeSrc->GetTime();
}
//---------------------------------------------------------------------

nTime CEntity::GetEntityTime() const
{
	nTime CurrTime = ExtTimeSrc.isvalid() ? ExtTimeSrc->GetTime() : GfxSrv->EntityTimeSrc->GetTime();
	nTime Time = TimeFactor * (CurrTime - ActivationTime);
	return n_max(0.0, Time);
}
//---------------------------------------------------------------------

void CEntity::UpdateGlobalBox()
{
	n_assert(Flags.Is(GLOBAL_BOX_DIRTY));
	Flags.Clear(GLOBAL_BOX_DIRTY);
	GlobalBBox = LocalBBox;
	GlobalBBox.transform(Transform);
}
//---------------------------------------------------------------------

bool CEntity::TestLODVisibility()
{
	// Check distance to viewer
	CCameraEntity* pCamera = GfxSrv->GetCamera();
	n_assert(pCamera);
	float Dist = vector3(pCamera->GetTransform().pos_component() - GetTransform().pos_component()).len();
	if (Dist > MaxVisibleDistance) FAIL;

	// Check screen space size
	const bbox3& WorldBB = GetBox();
	sphere WorldSphere(WorldBB.center(), WorldBB.diagonal_size() * 0.5f);
	rectangle ScreenRect = WorldSphere.project_screen_rh(pCamera->GetView(),
														 pCamera->GetCamera().GetProjection(),
														 pCamera->GetCamera().GetNearPlane());
	if (ScreenRect.width() < MinVisibleSize && ScreenRect.height() < MinVisibleSize) FAIL;

	OK;
}
//---------------------------------------------------------------------

EClipStatus CEntity::GetBoxClipStatus(const bbox3& Box)
{
	return GetBox().clipstatus(Box);
}
//---------------------------------------------------------------------

CGfxQTNode::~CGfxQTNode()
{
	for (int i = 0; i < GFXEntityNumTypes; i++)
	{
		CGfxEntityListSet::CElement* pCurr;
		while (pCurr = Entities.GetHead(EEntityType(i)))
		{
	//		UpdateNumEntitiesInHierarchy(Ents[j]->GetType(), -1);
			pCurr->Object->GetLevel()->RemoveEntity(pCurr->Object);
		}
	}

	//n_assert(!pNode->TotalObjCount);
}
//---------------------------------------------------------------------

bool CGfxQTNode::Remove(const PEntity& Object)
{
	//UpdateNumEntitiesInHierarchy((*ItEntity)->GetType(), -1);
	//Object->ClearLinks(AllLinks);
	return Entities.Remove(Object);
}
//---------------------------------------------------------------------

void CGfxQTNode::RemoveElement(CElement* pElement)
{
	//UpdateNumEntitiesInHierarchy((*ItEntity)->GetType(), -1);
	//pElement->Object->ClearLinks(AllLinks);
	Entities.RemoveElement(pElement);
}
//---------------------------------------------------------------------

//void CGfxQTNode::UpdateNumEntitiesInHierarchy(EEntityType Type, int Count)
//{
//	n_assert(Type >= 0 && Type < CEntity::NumTypes);
//
//	NumEntInHierarchyAllTypes += Count;
//	EntityCountByType[Type] += Count;
//	n_assert(NumEntInHierarchyAllTypes >= 0);
//	CCell* pCell = pParent;
//	if (pCell) do
//	{
//		pCell->NumEntInHierarchyAllTypes += Count;
//		pCell->EntityCountByType[Type] += Count;
//		n_assert(pCell->NumEntInHierarchyAllTypes >= 0);
//	}
//	while ((pCell = pCell->pParent));
//}
//---------------------------------------------------------------------

} // namespace Graphics
