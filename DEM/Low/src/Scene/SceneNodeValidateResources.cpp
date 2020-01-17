#include "SceneNodeValidateResources.h"
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>

namespace Scene
{

bool CSceneNodeValidateResources::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		Node.GetAttribute(i)->ValidateResources(_ResMgr);
	OK;
}
//--------------------------------------------------------------------

}
