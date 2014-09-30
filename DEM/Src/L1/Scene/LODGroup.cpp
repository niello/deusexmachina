#include "LODGroup.h"

#include <Scene/SceneNode.h>
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
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			SqThresholds.BeginAdd(Count);
			for (short i = 0; i < Count; ++i)
			{
				float Threshold;
				CStrID ChildID;
				DataReader.Read<float>(Threshold);
				DataReader.Read<CStrID>(ChildID);
				//!!!check FLT_MAX and what if square it!
				SqThresholds.Add(Threshold * Threshold, ChildID);
			}
			SqThresholds.EndAdd();
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CLODGroup::Update(const vector3* pCOIArray, DWORD COICount)
{
	if (!pNode || !pCOIArray || !COICount) return;

	// Select minimal distance, if there are multiple COIs
	const vector3& NodePos = pNode->GetWorldPosition();
	float SqDistance = FLT_MAX;
	for (DWORD i = 0; i < COICount; ++i)
	{
		vector3 DistanceVector = NodePos - pCOIArray[i];
		float SqDist = DistanceVector.SqLength();
		if (SqDistance > SqDist) SqDistance = SqDist;
	}

	CStrID SelectedChild;
	for (int i = 0; i < SqThresholds.GetCount(); ++i)
		if (SqThresholds.KeyAt(i) > SqDistance) SelectedChild = SqThresholds.ValueAt(i);
		else break;

	for (DWORD i = 0; i < pNode->GetChildCount(); ++i)
	{
		CSceneNode& Node = *pNode->GetChild(i);
		Node.Activate(Node.GetName() == SelectedChild);
	}
}
//---------------------------------------------------------------------

}