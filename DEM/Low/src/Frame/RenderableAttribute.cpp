#include "RenderableAttribute.h"
#include <Scene/SceneNode.h>
#include <Math/SIMDMath.h>
#include <Debug/DebugDraw.h>

namespace Frame
{

void CRenderableAttribute::UpdateInGraphicsScene(CGraphicsScene& Scene)
{
	n_assert_dbg(IsActive());

	Math::CAABB AABB;
	const bool IsAABBValid = GetLocalAABB(AABB);
	const bool SceneChanged = (_pScene != &Scene);

	if (_pScene && (SceneChanged || !IsAABBValid))
		_pScene->RemoveRenderable(_SceneRecordHandle);

	if (!IsAABBValid)
	{
		// This object currently has no bounds at all, it can't be added to the level
		_pScene = nullptr;
		_SceneRecordHandle = {};
	}
	else if (SceneChanged)
	{
		_pScene = &Scene;
		AABB = Math::AABBFromOBB(AABB, _pNode->GetWorldMatrix());
		_SceneRecordHandle = Scene.AddRenderable(AABB, *this);
		_LastTransformVersion = _pNode->GetTransformVersion();
	}
	else if (_pNode->GetTransformVersion() != _LastTransformVersion) //!!! || LocalBox changed!
	{
		AABB = Math::AABBFromOBB(AABB, _pNode->GetWorldMatrix());
		Scene.UpdateRenderableBounds(_SceneRecordHandle, AABB);
		_LastTransformVersion = _pNode->GetTransformVersion();
	}
}
//---------------------------------------------------------------------

bool CRenderableAttribute::GetGlobalAABB(Math::CAABB& OutBox, UPTR LOD) const
{
	if (!_pNode) return false;

	if (_pScene && _pNode->GetTransformVersion() == _LastTransformVersion) //!!! && LocalBox not changed!
	{
		OutBox = _SceneRecordHandle->second.Box;
	}
	else
	{
		if (GetLocalAABB(OutBox, LOD)) return false;
		OutBox = Math::AABBFromOBB(OutBox, _pNode->GetWorldMatrix());
	}

	return true;
}
//---------------------------------------------------------------------

void CRenderableAttribute::OnActivityChanged(bool Active)
{
	if (!Active && _pScene)
	{
		_pScene->RemoveRenderable(_SceneRecordHandle);
		_pScene = nullptr;
		_SceneRecordHandle = {};
	}
}
//---------------------------------------------------------------------

void CRenderableAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	// Draw world-space AABB
	Math::CAABB AABB;
	if (GetGlobalAABB(AABB) && Math::IsAABBValid(AABB))
		DebugDraw.DrawBoxWireframe(AABB, Render::ColorRGBA(160, 220, 255, 255), 1.f);

	// Draw spatial tree node bounds
	if (_pScene && _SceneRecordHandle->second.NodeIndex != NO_SPATIAL_TREE_NODE)
		DebugDraw.DrawBoxWireframe(_pScene->GetNodeAABB(_SceneRecordHandle->second.NodeIndex, true), Render::ColorRGBA(160, 255, 160, 255), 1.f);
}
//---------------------------------------------------------------------

}
