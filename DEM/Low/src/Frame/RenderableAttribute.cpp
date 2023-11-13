#include "RenderableAttribute.h"
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>

namespace Frame
{

void CRenderableAttribute::UpdateInGraphicsScene(CGraphicsScene& Scene)
{
	n_assert_dbg(IsActive());

	CAABB AABB;
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
		AABB.Transform(_pNode->GetWorldMatrix());
		_SceneRecordHandle = Scene.AddRenderable(AABB, *this);
		_LastTransformVersion = _pNode->GetTransformVersion();
	}
	else if (_pNode->GetTransformVersion() != _LastTransformVersion) //!!! || LocalBox changed!
	{
		AABB.Transform(_pNode->GetWorldMatrix());
		Scene.UpdateRenderableBounds(_SceneRecordHandle, AABB);
		_LastTransformVersion = _pNode->GetTransformVersion();
	}
}
//---------------------------------------------------------------------

bool CRenderableAttribute::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_pNode) return false;

	if (_pScene && _pNode->GetTransformVersion() == _LastTransformVersion) //!!! && LocalBox not changed!
	{
		// TODO: use Center+Extents SIMD AABB everywhere?!
		const auto Center = _SceneRecordHandle->second.BoxCenter;
		const auto Extent = _SceneRecordHandle->second.BoxExtent;
		OutBox.Set(
			vector3(rtm::vector_get_x(Center), rtm::vector_get_y(Center), rtm::vector_get_z(Center)),
			vector3(rtm::vector_get_x(Extent), rtm::vector_get_y(Extent), rtm::vector_get_z(Extent)));
	}
	else
	{
		if (GetLocalAABB(OutBox, LOD)) return false;
		OutBox.Transform(_pNode->GetWorldMatrix());
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
	CAABB AABB;
	if (GetGlobalAABB(AABB))
		DebugDraw.DrawBoxWireframe(AABB, Render::ColorRGBA(160, 220, 255, 255), 1.f);

	// Draw spatial tree node bounds
	if (_pScene)
		DebugDraw.DrawBoxWireframe(_pScene->GetNodeAABB(_SceneRecordHandle->second.NodeIndex, true), Render::ColorRGBA(160, 255, 160, 255), 1.f);
}
//---------------------------------------------------------------------

}
