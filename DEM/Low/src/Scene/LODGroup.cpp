#include "LODGroup.h"

#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Scene
{
FACTORY_CLASS_IMPL(Scene::CLODGroup, 'LODG', Scene::CNodeAttribute);

bool CLODGroup::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'TRSH':
			{
				// FIXME: must store last LOD (what is rendered after the last threshold).
				// Nodes are 1 more than thresholds, if min and max distances aren't used.

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
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CLODGroup::UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount)
{
	if (!_pNode || !pCOIArray || !COICount) return;

	// Select minimal distance, if there are multiple COIs
	const vector3& NodePos = _pNode->GetWorldPosition();
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

	for (UPTR i = 0; i < _pNode->GetChildCount(); ++i)
	{
		CSceneNode& Node = *_pNode->GetChild(i);
		Node.SetActive(Node.GetName() == SelectedChild);
	}
}
//---------------------------------------------------------------------

}