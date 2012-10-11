#include <StdAPI.h>
#include <App/CIDEApp.h>
#include <App/CSharpUIEventHandler.h>

API void SetUICallbacks(CCallback_V_S EntitySelected, CMouseButtonCallback MouseButton)
{
	CIDEApp->GetUIEventHandler()->OnEntitySelectedCB = EntitySelected;
	CIDEApp->GetUIEventHandler()->MouseButtonCB = MouseButton;
}
//---------------------------------------------------------------------
