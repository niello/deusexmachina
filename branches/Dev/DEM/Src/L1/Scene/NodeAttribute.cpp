#include "NodeAttribute.h"

#include <Scene/SceneNode.h>

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeAttribute, Core::CObject);

void CNodeAttribute::Update()
{
	Flags.SetTo(WorldMatrixChanged, pNode && pNode->IsWorldMatrixChanged());
}
//---------------------------------------------------------------------

void CNodeAttribute::RemoveFromNode()
{
	if (pNode) pNode->RemoveAttr(*this);
}
//---------------------------------------------------------------------

}