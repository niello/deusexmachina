#include "PropTransformable.h"

#include <Physics/Event/SetTransform.h>
#include <Events/EventBase.h>
#include <Game/Entity.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>
#include <gfx2/ngfxserver2.h>

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
	static const vector4 ColorX(1.0f, 0.0f, 0.0f, 1.0f);
	static const vector4 ColorY(0.0f, 1.0f, 0.0f, 1.0f);
	static const vector4 ColorZ(0.0f, 0.0f, 1.0f, 1.0f);

	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);

	nFixedArray<vector3> lines(2);
	lines[1].x = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, Tfm, ColorX);
	lines[1].x = 0.f;
	lines[1].y = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, Tfm, ColorY);
	lines[1].y = 0.f;
	lines[1].z = 1.f;
	nGfxServer2::Instance()->DrawShapePrimitives(nGfxServer2::LineList, 1, &(lines[0]), 3, Tfm, ColorZ);
}
//---------------------------------------------------------------------

} // namespace Properties
