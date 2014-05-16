#include "NodeController.h"

#include <Scene/SceneNode.h>
#include <Scene/NodeControllerComposite.h>

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeController, Core::CObject);

void CNodeController::RemoveFromNode()
{
	if (!pNode) return;
	n_assert_dbg(pNode->GetController());
	if (pNode->GetController() == this) pNode->SetController(NULL);
	else if (pNode->GetController()->IsA<CNodeControllerComposite>())
	{
		n_verify_dbg(((CNodeControllerComposite*)pNode->GetController())->RemoveSource(*this));
	}
	if (pNode) Sys::Error("Attached node controller was not found in the host node!");
}
//---------------------------------------------------------------------

}