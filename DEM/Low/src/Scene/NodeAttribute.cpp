#include "NodeAttribute.h"

#include <Scene/SceneNode.h>

namespace Scene
{
RTTI_CLASS_IMPL(Scene::CNodeAttribute, Core::CObject);

void CNodeAttribute::RemoveFromNode()
{
	if (pNode) pNode->RemoveAttribute(*this);
}
//---------------------------------------------------------------------

}