#include "RenderableAttribute.h"
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
#include <acl/math/vector4_32.h>

namespace Frame
{

void CRenderableAttribute::UpdateInGraphicsScene(CGraphicsScene& Scene)
{
	n_assert_dbg(IsActive());

	CAABB AABB;
	const bool IsAABBValid = GetLocalAABB(AABB);
	const bool SceneChanged = (pScene != &Scene);

	if (pScene && (SceneChanged || !IsAABBValid))
		pScene->RemoveRenderable(SceneRecordHandle);

	if (!IsAABBValid)
	{
		// This object currently has no bounds at all, it can't be added to the level
		pScene = nullptr;
		SceneRecordHandle = {};
	}
	else if (SceneChanged)
	{
		pScene = &Scene;
		AABB.Transform(_pNode->GetWorldMatrix());
		SceneRecordHandle = Scene.AddRenderable(AABB, *this);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
	else if (_pNode->GetTransformVersion() != LastTransformVersion) //!!! || LocalBox changed!
	{
		AABB.Transform(_pNode->GetWorldMatrix());
		Scene.UpdateRenderable(SceneRecordHandle, AABB);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
}
//---------------------------------------------------------------------

bool CRenderableAttribute::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_pNode) return false;

	if (pScene && _pNode->GetTransformVersion() == LastTransformVersion) //!!! && LocalBox not changed!
	{
		// TODO: use Center+Extents SIMD AABB everywhere?!
		const auto Center = SceneRecordHandle->second.BoxCenter;
		const auto Extent = SceneRecordHandle->second.BoxExtent;
		OutBox.Set(
			vector3(acl::vector_get_x(Center), acl::vector_get_y(Center), acl::vector_get_z(Center)),
			vector3(acl::vector_get_x(Extent), acl::vector_get_y(Extent), acl::vector_get_z(Extent)));
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
	if (!Active && pScene)
	{
		pScene->RemoveRenderable(SceneRecordHandle);
		pScene = nullptr;
		SceneRecordHandle = {};
	}
}
//---------------------------------------------------------------------

void CRenderableAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	CAABB AABB;
	if (GetGlobalAABB(AABB))
		DebugDraw.DrawBoxWireframe(AABB, Render::ColorRGBA(160, 220, 255, 255), 1.f);

	if (pScene)
		DebugDraw.DrawBoxWireframe(pScene->GetNodeAABB(SceneRecordHandle->second.NodeIndex, true), Render::ColorRGBA(160, 255, 160, 255), 1.f);
}
//---------------------------------------------------------------------

}
