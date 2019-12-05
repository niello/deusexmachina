#include "NodeController.h"

#include <Scene/SceneNode.h>
#include <Scene/NodeControllerComposite.h>

namespace Scene
{
RTTI_CLASS_IMPL(Scene::CNodeController, Core::CObject);

void CNodeController::RemoveFromNode()
{
	if (!pNode) return;
	n_assert_dbg(pNode->GetController());
	if (pNode->GetController() == this) pNode->SetController(nullptr);
	else if (pNode->GetController()->IsA<CNodeControllerComposite>())
	{
		n_verify_dbg(((CNodeControllerComposite*)pNode->GetController())->RemoveSource(*this));
	}
	if (pNode) Sys::Error("Attached node controller was not found in the host node!");
}
//---------------------------------------------------------------------

}