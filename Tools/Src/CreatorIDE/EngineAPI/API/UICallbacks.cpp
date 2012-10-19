#include <StdAPI.h>
#include <App/CIDEApp.h>
#include <App/CSharpUIConnector.h>

API void SetUICallbacks(CCallback_V_S EntitySelected,
						CMouseButtonCallback MouseButton,
						CCallback_S_S StringInput)
{
	CIDEApp->GetUIEventHandler()->OnEntitySelectedCB = EntitySelected;
	CIDEApp->GetUIEventHandler()->MouseButtonCB = MouseButton;
	CIDEApp->GetUIEventHandler()->StringInputCB = StringInput;
}
//---------------------------------------------------------------------
