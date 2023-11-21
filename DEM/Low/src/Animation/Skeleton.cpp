#include "Skeleton.h"
#include <Animation/SkeletonInfo.h>
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{

void CSkeleton::Init(Scene::CSceneNode& Root, const CSkeletonInfo& Info)
{
	_Nodes.clear();
	const auto NodeCount = Info.GetNodeCount();

	if (!NodeCount) return;

	_Nodes.resize(NodeCount);
	_Nodes[0] = &Root;

	for (UPTR i = 1; i < NodeCount; ++i)
	{
		const auto& NodeInfo = Info.GetNodeInfo(i);

		Scene::CSceneNode* pNode = nullptr;
		if (NodeInfo.ParentIndex == CSkeletonInfo::EmptyPort)
		{
			// When no parent is specified, we search by path from the root node
			_Nodes[i] = _Nodes[0]->FindNodeByPath(NodeInfo.ID.CStr());
		}
		else if (auto pParent = _Nodes[NodeInfo.ParentIndex].Get())
		{
			// When parent is specified, the node is its direct child
			_Nodes[i] = pParent->GetChild(NodeInfo.ID);
		}
	}
}
//---------------------------------------------------------------------

U16 CSkeleton::FindPortByName(CStrID NodeID) const
{
	const U16 Size = static_cast<U16>(_Nodes.size());
	for (U16 i = 0; i < Size; ++i)
		if (_Nodes[i] && _Nodes[i]->GetName() == NodeID)
			return i;
	return InvalidPort;
}
//---------------------------------------------------------------------

void CSkeleton::FromPoseBuffer(const CPoseBuffer& Pose)
{
	const UPTR Size = _Nodes.size();
	for (UPTR i = 0; i < Size; ++i)
		if (auto pNode = _Nodes[i].Get())
			pNode->SetLocalTransform(Pose[i]);
}
//---------------------------------------------------------------------

void CSkeleton::ToPoseBuffer(CPoseBuffer& Pose) const
{
	const UPTR Size = _Nodes.size();
	Pose.SetSize(Size);
	for (UPTR i = 0; i < Size; ++i)
		if (auto pNode = _Nodes[i].Get())
			Pose[i] = pNode->GetLocalTransform();
}
//---------------------------------------------------------------------

U8 CSkeleton::GetActivePortChannels(U16 Port) const
{
	const bool NodeActive = (Port < _Nodes.size() && _Nodes[Port] && _Nodes[Port]->IsActive());
	return NodeActive ? ETransformChannel::All : 0;
}
//---------------------------------------------------------------------

}
