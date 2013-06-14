#include "NodeController.h"

#include <Scene/SceneNode.h>
#include <Animation/NodeControllerPriorityBlend.h>

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeController, Core::CRefCounted);

void CNodeController::RemoveFromNode()
{
	if (pNode) //pNode->RemoveController(*this);
	{
		n_assert_dbg(pNode->GetController());
		if (pNode->GetController() == this) pNode->SetController(NULL);
		else if (pNode->GetController()->IsComposite())
			((Anim::CNodeControllerPriorityBlend*)pNode->GetController())->RemoveSource(*this);
	}
}
//---------------------------------------------------------------------

}