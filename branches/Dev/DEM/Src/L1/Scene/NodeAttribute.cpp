#include "NodeAttribute.h"

#include <Scene/SceneNode.h>

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeAttribute, Core::CObject);

void CNodeAttribute::Update()
{
	if (pNode && pNode->IsWorldMatrixChanged()) Flags.Set(WorldMatrixChanged);
}
//---------------------------------------------------------------------

void CNodeAttribute::RemoveFromNode()
{
	if (pNode) pNode->RemoveAttr(*this);
}
//---------------------------------------------------------------------

}