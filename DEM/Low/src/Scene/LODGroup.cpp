#include "LODGroup.h"

#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Scene
{
__ImplementClass(Scene::CLODGroup, 'LODG', Scene::CNodeAttribute);

bool CLODGroup::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
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
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

PNodeAttribute CLODGroup::Clone()
{
	PLODGroup ClonedAttr = n_new(CLODGroup);
	SqThresholds.Copy(ClonedAttr->SqThresholds);
	return ClonedAttr.Get();
}
//---------------------------------------------------------------------

void CLODGroup::Update(const vector3* pCOIArray, UPTR COICount)
{
	if (!pNode || !pCOIArray || !COICount) return;

	// Select minimal distance, if there are multiple COIs
	const vector3& NodePos = pNode->GetWorldPosition();
	float SqDistance = FLT_MAX;
	for (UPTR i = 0; i < COICount; ++i)
	{
		vector3 DistanceVector = NodePos - pCOIArray[i];
		float SqDist = DistanceVector.SqLength();
		if (SqDistance > SqDist) SqDistance = SqDist;
	}

	CStrID SelectedChild;
	for (UPTR i = 0; i < SqThresholds.GetCount(); ++i)
	{
		if (SqThresholds.KeyAt(i) > SqDistance)
			SelectedChild = SqThresholds.ValueAt(i);
		else
			break;
	}

	for (UPTR i = 0; i < pNode->GetChildCount(); ++i)
	{
		CSceneNode& Node = *pNode->GetChild(i);
		Node.Activate(Node.GetName() == SelectedChild);
	}
}
//---------------------------------------------------------------------

}