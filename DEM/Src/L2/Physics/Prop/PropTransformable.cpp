#include "PropTransformable.h"

#include <Physics/Event/SetTransform.h>
#include <Events/EventBase.h>
#include <Game/Entity.h>
#include <Render/DebugDraw.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineMatrix44(Transform);
};

BEGIN_ATTRS_REGISTRATION(PropTransformable)
	RegisterMatrix44WithDefault(Transform, ReadWrite, matrix44::identity);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropTransformable, Game::CProperty);
ImplementFactory(Properties::CPropTransformable);
ImplementPropertyStorage(CPropTransformable, 512);
RegisterProperty(CPropTransformable);

IMPL_EVENT_HANDLER_VIRTUAL(OnRenderDebug, CPropTransformable, OnRenderDebug)

void CPropTransformable::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::Transform);
}
//---------------------------------------------------------------------

void CPropTransformable::Activate()
{
	CProperty::Activate();
	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropTransformable, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropTransformable, OnRenderDebugProc);
}
//---------------------------------------------------------------------

void CPropTransformable::Deactivate()
{
	UNSUBSCRIBE_EVENT(SetTransform);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropTransformable::OnSetTransform(const Events::CEventBase& Event)
{
	SetTransform(((Event::SetTransform&)Event).Transform);
	OK;
}
//---------------------------------------------------------------------

void CPropTransformable::SetTransform(const matrix44& NewTF)
{
	// Tfm-using props can cache ref to VT field where this matrix is to avoid lookup.
	// This is allowed only for reading, writing value requires Set call and occurs only here!
	GetEntity()->Set<matrix44>(Attr::Transform, NewTF);
	GetEntity()->FireEvent(CStrID("UpdateTransform"));
}
//---------------------------------------------------------------------

void CPropTransformable::OnRenderDebug()
{
	DebugDraw->DrawCoordAxes(GetEntity()->Get<matrix44>(Attr::Transform));
}
//---------------------------------------------------------------------

} // namespace Properties
