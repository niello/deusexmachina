#include "LODGroup.h"

#include <Scene/Scene.h>

namespace Scene
{
ImplementRTTI(Scene::CLODGroup, Scene::CSceneNodeAttr);

void CLODGroup::UpdateTransform(CScene& Scene)
{
	//???what if camera transform wasn't updated this frame yet?
	//Update camera branch of scene graph before?
	//Can use Updated flag and clear each frame, with clearing visible meshes and lights

	if (!pNode || !Scene.GetCurrCamera()) return;

	vector3 DistanceVector = pNode->GetWorldTransform().pos_component() -
		Scene.GetCurrCamera()->GetNode()->GetWorldTransform().pos_component();
	float SqDist = DistanceVector.lensquared();

	CStrID SelectedChild;
	if (SqDist >= MinSqDistance && SqDist <= MaxSqDistance)
		for (int i = 0; i < SqThresholds.Size(); ++i)
			if (SqThresholds.KeyAtIndex(i) > SqDist) SelectedChild = SqThresholds.ValueAtIndex(i);
			else break;

	for (DWORD i = 0; i < pNode->GetChildCount(); ++i)
	{
		CSceneNode& Node = *pNode->GetChild(i);
		if (Node.IsLODDependent())
			Node.Activate(Node.GetName() == SelectedChild);
	}
}
//---------------------------------------------------------------------

}