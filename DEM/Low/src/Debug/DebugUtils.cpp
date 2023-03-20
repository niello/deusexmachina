#include "DebugUtils.h"
#include <Scene/NodeAttribute.h>

namespace DEM::Debug
{

void PrintSceneNodeHierarchy(const Scene::CSceneNode& Node, size_t Indent)
{
	::Sys::DbgOut((std::string(Indent, ' ') + Node.GetName().ToString() + '\n').c_str());

	for (size_t i = 0; i < Node.GetAttributeCount(); ++i)
		::Sys::DbgOut((std::string(Indent, ' ') + '#' + Node.GetAttribute(i)->GetClassName() + '\n').c_str());

	for (size_t i = 0; i < Node.GetChildCount(); ++i)
		PrintSceneNodeHierarchy(*Node.GetChild(i), Indent + 1);
}
//---------------------------------------------------------------------

}
