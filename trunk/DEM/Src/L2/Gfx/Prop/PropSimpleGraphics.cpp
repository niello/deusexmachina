#include "PropSimpleGraphics.h"

#include <Gfx/GfxServer.h>
#include <Gfx/ShapeEntity.h>
#include <Game/Entity.h>
#include <Loading/EntityFactory.h>
#include <Physics/Prop/PropTransformable.h>

namespace Attr
{
	DeclareAttr(Graphics);
}

namespace Properties
{
ImplementRTTI(Properties::CPropSimpleGraphics, Properties::CPropAbstractGraphics);
ImplementFactory(Properties::CPropSimpleGraphics);
ImplementPropertyStorage(CPropSimpleGraphics, 64);
RegisterProperty(CPropSimpleGraphics);

CPropSimpleGraphics::CPropSimpleGraphics()
{
}
//---------------------------------------------------------------------

CPropSimpleGraphics::~CPropSimpleGraphics()
{
	n_assert(!GfxEntity.isvalid());
}
//---------------------------------------------------------------------

void CPropSimpleGraphics::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropAbstractGraphics::GetAttributes(Attrs);
	Attrs.Append(Attr::Graphics);
}
//---------------------------------------------------------------------

void CPropSimpleGraphics::Activate()
{
	CPropAbstractGraphics::Activate();
	
	n_assert(GetEntity()->HasAttr(Attr::Transform)); //???need?
	n_assert(!GfxEntity.isvalid());

	GfxEntity = Graphics::CShapeEntity::Create();
	GfxEntity->SetUserData(GetEntity()->GetUniqueID());
	GfxEntity->SetResourceName(GetGraphicsResource());
	GfxEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));

	Graphics::CLevel* GfxLevel = GfxSrv->GetLevel();
	n_assert(GfxLevel);
	GfxLevel->AttachEntity(GfxEntity);

	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropSimpleGraphics, OnUpdateTransform);
	PROP_SUBSCRIBE_PEVENT(GfxSetVisible, CPropSimpleGraphics, OnGfxSetVisible);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropSimpleGraphics, OnEntityRenamed);
}
//---------------------------------------------------------------------

void CPropSimpleGraphics::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);
	UNSUBSCRIBE_EVENT(GfxSetVisible);
	UNSUBSCRIBE_EVENT(OnEntityRenamed);

	if (GfxEntity.isvalid())
	{
		Graphics::CLevel* GfxLevel = GfxSrv->GetLevel();
		n_assert(GfxLevel);
		GfxLevel->RemoveEntity(GfxEntity);
		GfxEntity = NULL;
	}

	CPropAbstractGraphics::Deactivate();
}
//---------------------------------------------------------------------

bool CPropSimpleGraphics::OnUpdateTransform(const Events::CEventBase& Event)
{
	//???cache Attr::Transform matrix ref-ptr?
	if (GfxEntity.isvalid())
		GfxEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	//((const Event::UpdateTransform&)Event).Transform);
	OK;
}
//---------------------------------------------------------------------

bool CPropSimpleGraphics::OnGfxSetVisible(const Events::CEventBase& Event)
{
	if (GfxEntity.isvalid())
		GfxEntity->SetVisible(((const Events::CEvent&)Event).Params->Get<bool>(CStrID("Visible"), true));
	OK;
}
//---------------------------------------------------------------------

bool CPropSimpleGraphics::OnEntityRenamed(const Events::CEventBase& Event)
{
	if (GfxEntity.isvalid()) GfxEntity->SetUserData(GetEntity()->GetUniqueID());
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
