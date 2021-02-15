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
	// TODO: scene nodes to ACL/RTM?
	Math::CTransform Tfm;

	const UPTR Size = _Nodes.size();
	for (UPTR i = 0; i < Size; ++i)
	{
		if (auto pNode = _Nodes[i].Get())
		{
			const auto& SrcTfm = Pose[i];
			Tfm.Scale.set(acl::vector_get_x(SrcTfm.scale), acl::vector_get_y(SrcTfm.scale), acl::vector_get_z(SrcTfm.scale));
			Tfm.Rotation.set(acl::vector_get_x(SrcTfm.rotation), acl::vector_get_y(SrcTfm.rotation), acl::vector_get_z(SrcTfm.rotation), acl::vector_get_w(SrcTfm.rotation));
			Tfm.Translation.set(acl::vector_get_x(SrcTfm.translation), acl::vector_get_y(SrcTfm.translation), acl::vector_get_z(SrcTfm.translation));
			pNode->SetLocalTransform(Tfm);
		}
	}
}
//---------------------------------------------------------------------

void CSkeleton::ToPoseBuffer(CPoseBuffer& Pose) const
{
	const UPTR Size = _Nodes.size();
	Pose.SetSize(Size);
	for (UPTR i = 0; i < Size; ++i)
	{
		if (auto pNode = _Nodes[i].Get())
		{
			const auto& SrcTfm = pNode->GetLocalTransform();
			Pose[i].scale = acl::vector_set(SrcTfm.Scale.x, SrcTfm.Scale.y, SrcTfm.Scale.z);
			Pose[i].rotation = acl::quat_set(SrcTfm.Rotation.x, SrcTfm.Rotation.y, SrcTfm.Rotation.z, SrcTfm.Rotation.w);
			Pose[i].translation = acl::vector_set(SrcTfm.Translation.x, SrcTfm.Translation.y, SrcTfm.Translation.z);
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
