#include "PropCamera.h"

#include <Game/Mgr/FocusManager.h>
#include <Audio/AudioServer.h>

//BEGIN_ATTRS_REGISTRATION(PropCamera)
//	RegisterFloatWithDefault(FieldOfView, ReadOnly, 60.f);
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropCamera, 'PCAM', Game::CProperty);
__ImplementPropertyStorage(CPropCamera);

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

} // namespace Prop
