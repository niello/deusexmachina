#include "Skeleton.h"
#include <Animation/SkeletonInfo.h>

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

U8 CSkeleton::GetActivePortChannels(U16 Port) const
{
	const bool NodeActive = (Port < _Nodes.size() && _Nodes[Port] && _Nodes[Port]->IsActive());
	return NodeActive ? ETransformChannel::All : 0;
}
//---------------------------------------------------------------------

void CSkeleton::SetScale(U16 Port, const vector3& Scale)
{
	_Nodes[Port]->SetLocalScale(Scale);
}
//---------------------------------------------------------------------

void CSkeleton::SetRotation(U16 Port, const quaternion& Rotation)
{
	_Nodes[Port]->SetLocalRotation(Rotation);
}
//---------------------------------------------------------------------

void CSkeleton::SetTranslation(U16 Port, const vector3& Translation)
{
	_Nodes[Port]->SetLocalPosition(Translation);
}
//---------------------------------------------------------------------

void CSkeleton::SetTransform(U16 Port, const Math::CTransformSRT& Tfm)
{
	_Nodes[Port]->SetLocalTransform(Tfm);
}
//---------------------------------------------------------------------

}
