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
				for (short i = 0; i < Count; ++i)
				{
					float Threshold;
					CStrID ChildID;
					DataReader.Read<float>(Threshold);
					DataReader.Read<CStrID>(ChildID);
					//!!!check FLT_MAX and what if square it!
					SqThresholds.emplace(Threshold * Threshold, ChildID);
				}
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
	ClonedAttr->SqThresholds = SqThresholds;
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CLODGroup::UpdateBeforeChildren(const rtm::vector4f* pCOIArray, UPTR COICount)
{
	if (!_pNode || !pCOIArray || !COICount) return;

	// Select minimal distance, if there are multiple COIs
	const rtm::vector4f NodePos = _pNode->GetWorldPosition();
	float SqDistance = FLT_MAX;
	for (UPTR i = 0; i < COICount; ++i)
	{
		const rtm::vector4f DistanceVector = rtm::vector_sub(NodePos, pCOIArray[i]);
		const float CurrSqDistance = rtm::vector_length_squared3(DistanceVector);
		if (SqDistance > CurrSqDistance) SqDistance = CurrSqDistance;
	}

	CStrID SelectedChild;
	for (const auto [SqThreshold, ID] : SqThresholds)
	{
		if (SqThreshold > SqDistance)
			SelectedChild = ID;
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
