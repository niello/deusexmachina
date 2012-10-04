#include "PropGraphics.h"

#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Gfx/ShapeEntity.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Loading/EntityFactory.h>
#include <Gfx/PhysicsGfxUtil.h>

namespace Attr
{
	DeclareAttr(Graphics);
}


namespace Properties
{
ImplementRTTI(Properties::CPropGraphics, CPropAbstractGraphics);
ImplementFactory(Properties::CPropGraphics);
ImplementPropertyStorage(CPropGraphics, 512);
RegisterProperty(CPropGraphics);

CPropGraphics::~CPropGraphics()
{
	n_assert(GraphicsEntities.Size() == 0);
}
//---------------------------------------------------------------------

void CPropGraphics::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropAbstractGraphics::GetAttributes(Attrs);
	Attrs.Append(Attr::Graphics);
}
//---------------------------------------------------------------------

void CPropGraphics::Activate()
{
	CPropAbstractGraphics::Activate();
	
	SetupGraphicsEntities();

	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropGraphics, OnUpdateTransform);
	PROP_SUBSCRIBE_PEVENT(GfxSetVisible, CPropGraphics, OnGfxSetVisible);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropGraphics, OnEntityRenamed);
}
//---------------------------------------------------------------------

void CPropGraphics::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);
	UNSUBSCRIBE_EVENT(GfxSetVisible);
	UNSUBSCRIBE_EVENT(OnEntityRenamed);

	for (int i = 0;  i < GraphicsEntities.Size(); i++)
		GfxSrv->GetLevel()->RemoveEntity(GraphicsEntities[i].get_unsafe());

	GraphicsEntities.Clear();

	CPropAbstractGraphics::Deactivate();
}
//---------------------------------------------------------------------

void CPropGraphics::SetupGraphicsEntities()
{
	// get some entity attributes
	nString RsrcName = GetGraphicsResource();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!init correctly, phys entity tfm must correspond to game entity tfm!
	//for multiple physics objects should be implemented special code!

	//??? mangalore: TODO: DO NOT SEARCH FOR A ATTACHED PHYSICS PROERTY!
	// check if we have a physics property attached
	CPropAbstractPhysics* PhysProp = GetEntity()->FindProperty<CPropAbstractPhysics>();
	if (PhysProp && PhysProp->IsEnabled())
	{
		// setup graphics entities for visualizing physics
		Physics::CEntity* pPhysEnt = PhysProp->GetPhysicsEntity();
		if (!pPhysEnt) return;

		Graphics::CPhysicsGfxUtil::CreateGraphics(pPhysEnt, GraphicsEntities);
		n_assert(GraphicsEntities.Size() > 0);
		Graphics::CPhysicsGfxUtil::SetupGraphics(RsrcName, pPhysEnt, GraphicsEntities);
	}
	else
	{
		GfxSrv->CreateGfxEntities(RsrcName, GetEntity()->Get<matrix44>(Attr::Transform), GraphicsEntities);
		for (int i = 0; i < GraphicsEntities.Size(); i++)
			GfxSrv->GetLevel()->AttachEntity(GraphicsEntities[i]);
	}

	for (int i = 0; i < GraphicsEntities.Size(); i++)
		GraphicsEntities[i]->SetUserData(GetEntity()->GetUniqueID());
}
//---------------------------------------------------------------------

void CPropGraphics::GetAABB(bbox3& AABB) const
{
	if (GraphicsEntities.Size() > 0)
	{
		AABB = GraphicsEntities[0]->GetBox();
		for (int i = 1; i < GraphicsEntities.Size(); i++)
			AABB.extend(GraphicsEntities[i]->GetBox());
	}
	else
	{
		AABB.vmin.x = 
		AABB.vmin.y = 
		AABB.vmin.z = 
		AABB.vmax.x = 
		AABB.vmax.y = 
		AABB.vmax.z = 0.f;
	}
}
//---------------------------------------------------------------------

void CPropGraphics::UpdateTransform()
{
	//???cache Attr::Transform matrix ref-ptr?
	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);
	for (int i = 0; i < GraphicsEntities.Size(); i++)
		GraphicsEntities[i]->SetTransform(Tfm);

	////!!!for multiple physics objects should be implemented special code!
	//// SPECIAL CASES ARE:
	//// physicsEntity->GetComposite()->IsA(Physics::Ragdoll::RTTI) or
	//// physicsEntity->GetComposite()->HasLinkType(Physics::RigidBody::ModelNode)
	//if (physProperty->IsEnabled()) // gather transform from physics entity
	//{
	//	Physics::CEntity* pPhysEnt = physProperty->GetPhysicsEntity();
	//	if (pPhysEnt)
	//		Util::CPhysicsGfxUtil::UpdateGraphicsTransforms(pPhysEnt, GraphicsEntities);
	//}
}
//---------------------------------------------------------------------

bool CPropGraphics::OnUpdateTransform(const CEventBase& Event)
{
	UpdateTransform();
	OK;
}
//---------------------------------------------------------------------

bool CPropGraphics::OnGfxSetVisible(const CEventBase& Event)
{
	bool Visible = ((const Events::CEvent&)Event).Params->Get<bool>(CStrID("Visible"), true);
	for (int i = 0; i < GraphicsEntities.Size(); i++)
		GraphicsEntities[i]->SetVisible(Visible);
	OK;
}
//---------------------------------------------------------------------

bool CPropGraphics::OnEntityRenamed(const CEventBase& Event)
{
	for (int i = 0; i < GraphicsEntities.Size(); i++)
		GraphicsEntities[i]->SetUserData(GetEntity()->GetUniqueID());
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
