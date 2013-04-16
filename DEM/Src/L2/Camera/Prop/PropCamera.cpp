#include "PropCamera.h"

#include <Game/Mgr/FocusManager.h>
#include <Audio/AudioServer.h>
#include <DB/DBServer.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DefineFloat(FieldOfView);
};

BEGIN_ATTRS_REGISTRATION(PropCamera)
	RegisterFloatWithDefault(FieldOfView, ReadOnly, 60.f);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropCamera, Game::CProperty);
ImplementFactory(Properties::CPropCamera);
ImplementPropertyStorage(CPropCamera, 16);
RegisterProperty(CPropCamera);

using namespace Game;

IMPL_EVENT_HANDLER_VIRTUAL(OnRender, CPropCamera, OnRender)
IMPL_EVENT_HANDLER_VIRTUAL(OnObtainCameraFocus, CPropCamera, OnObtainCameraFocus)
IMPL_EVENT_HANDLER_VIRTUAL(OnLoseCameraFocus, CPropCamera, OnLoseCameraFocus)

CPropCamera::CPropCamera()
{
	//ShakeFxHelper.SetMaxDisplacement(vector3(0.5f, 0.5f, 0.25f));
	//ShakeFxHelper.SetMaxTumble(vector3(5.0f, 5.0f, 5.0f));
}
//---------------------------------------------------------------------

void CPropCamera::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::FieldOfView);
}
//---------------------------------------------------------------------

void CPropCamera::Activate()
{
	CProperty::Activate();

	PROP_SUBSCRIBE_PEVENT(OnObtainCameraFocus, CPropCamera, OnObtainCameraFocusProc);
	PROP_SUBSCRIBE_PEVENT(OnLoseCameraFocus, CPropCamera, OnLoseCameraFocusProc);

	//???can ever be? or FocusMgr sets focus once on game start?
	if (HasFocus()) PROP_SUBSCRIBE_PEVENT(OnRender, CPropCamera, OnRenderProc);
}
//---------------------------------------------------------------------

void CPropCamera::Deactivate()
{
    if (HasFocus())
	{
		UNSUBSCRIBE_EVENT(OnRender);
		FocusMgr->SetCameraFocusEntity(NULL);
	}

	UNSUBSCRIBE_EVENT(OnObtainCameraFocus);
	UNSUBSCRIBE_EVENT(OnLoseCameraFocus);

    CProperty::Deactivate();
}
//---------------------------------------------------------------------

void CPropCamera::OnObtainCameraFocus()
{
	PROP_SUBSCRIBE_PEVENT(OnRender, CPropCamera, OnRenderProc);
}
//---------------------------------------------------------------------

void CPropCamera::OnLoseCameraFocus()
{
	UNSUBSCRIBE_EVENT(OnRender);
}
//---------------------------------------------------------------------

void CPropCamera::OnRender()
{
	//ShakeFxHelper.SetCameraTransform(RenderSrv->GetDisplay().GetCamera()->GetTransform());
	//ShakeFxHelper.Update();
	//GetCamera()->SetTransform(ShakeFxHelper.GetShakeCameraTransform());
	//AudioSrv->ListenerTransform = GetCamera()->GetTransform();
}
//---------------------------------------------------------------------

bool CPropCamera::HasFocus() const
{
	return FocusMgr->GetCameraFocusEntity() == GetEntity();
}
//---------------------------------------------------------------------

} // namespace Properties
