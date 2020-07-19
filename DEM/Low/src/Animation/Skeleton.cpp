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
	// Return root port is no parent is specified
	if (ParentPort == InvalidPort) return _Nodes.empty() ? InvalidPort : 0;

	const Scene::CSceneNode* pParent = _Nodes[ParentPort].Get();
	if (!pParent) return InvalidPort;

	Scene::CSceneNode* pNode = pParent->GetChild(NodeID);
	if (!pNode) return InvalidPort;

	// Check if this node is already bound to some port
	auto It = std::find(_Nodes.cbegin(), _Nodes.cend(), pNode);
	if (It != _Nodes.cend()) return static_cast<U16>(std::distance(_Nodes.cbegin(), It));

	// Create new port, if not
	_Nodes.push_back(pNode);
	return static_cast<U16>(_Nodes.size() - 1);
}
//---------------------------------------------------------------------

bool CSkeleton::IsPortActive(U16 Port) const
{
	return _Nodes.size() > Port && _Nodes[Port] && _Nodes[Port]->IsActive();
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
