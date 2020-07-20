#pragma once
#include "Skeleton.h"

namespace DEM::Anim
{

void CSkeleton::SetRootNode(Scene::CSceneNode* pNode)
{
	if (_Nodes.empty())
	{
		if (pNode) _Nodes.push_back(pNode);
	}
	else _Nodes[0] = pNode;

	//!!!could rebind instead of invalidating!
	//!!!to rebind, needs to remember name+parent when port is created!
	for (size_t i = 1; i < _Nodes.size(); ++i)
		_Nodes[i] = nullptr;
}
//---------------------------------------------------------------------

U16 CSkeleton::BindNode(CStrID NodeID, U16 ParentPort)
{
	// Root must be set before binding other nodes
	if (_Nodes.empty() || !_Nodes[0]) return InvalidPort;

	Scene::CSceneNode* pNode = nullptr;
	if (ParentPort == InvalidPort)
	{
		// When no parent specified, we search by path from the root node.
		// Empty path is the root node itself, and root port is always 0.
		// FIXME: either require name matching or always store empty ID for clip root in DEM ACL files!
		if (!NodeID || NodeID == _Nodes[0]->GetName()) return 0;
		pNode = _Nodes[0]->FindNodeByPath(NodeID.CStr());
	}
	else if (auto pParent = _Nodes[ParentPort].Get())
	{
		// When parent is specified, the node is its direct child
		pNode = pParent->GetChild(NodeID);
	}

	if (!pNode) return InvalidPort;

	// Check if this node is already bound to some port
	auto It = std::find(_Nodes.cbegin(), _Nodes.cend(), pNode);
	if (It != _Nodes.cend()) return static_cast<U16>(std::distance(_Nodes.cbegin(), It));

	// Create new port, if not
	_Nodes.push_back(pNode);
	return static_cast<U16>(_Nodes.size() - 1);
}
//---------------------------------------------------------------------

U8 CSkeleton::GetActivePortChannels(U16 Port) const
{
	const bool NodeActive = (_Nodes.size() > Port && _Nodes[Port] && _Nodes[Port]->IsActive());
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
