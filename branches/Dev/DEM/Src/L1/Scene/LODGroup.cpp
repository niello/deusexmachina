#include "LODGroup.h"

#include <Scene/Scene.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Scene
{
__ImplementClass(Scene::CLODGroup, 'LODG', Scene::CNodeAttribute);

bool CLODGroup::LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader)
{
	switch (FourCC.Code)
	{
		case 'TRSH':
		{
			//!!!node ID!

			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				float SqThreshold;
				DataReader.Read<float>(SqThreshold);
				SqThreshold *= SqThreshold;
				//SqThresholds.Add(SqThreshold, ChildID);
				//!!!it is normal to have CStrID::Empty as ID - LOD assumes no children!
			}
			OK;
		}
		case 'DMIN':
		{
			DataReader.Read<float>(MinSqDistance);
			MinSqDistance *= MinSqDistance;
			OK;
		}
		case 'DMAX':
		{
			DataReader.Read<float>(MaxSqDistance);
			MaxSqDistance *= MaxSqDistance;
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLODGroup::Update()
{
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//???what if camera transform wasn't updated this frame yet?
	//Update camera branch of scene graph before?
	//Can use Updated(ThisFrame) flag and clear at each frame ending, with clearing visible meshes and lights
	//Flag Changed(ThisFrame) is smth different but may be usable too

	if (!pNode) return;

	vector3 DistanceVector = pNode->GetWorldPosition() - pNode->GetScene()->GetMainCamera().GetPosition();
	float SqDist = DistanceVector.SqLength();

	CStrID SelectedChild;
	if (SqDist >= MinSqDistance && SqDist <= MaxSqDistance)
		for (int i = 0; i < SqThresholds.GetCount(); ++i)
			if (SqThresholds.KeyAt(i) > SqDist) SelectedChild = SqThresholds.ValueAt(i);
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