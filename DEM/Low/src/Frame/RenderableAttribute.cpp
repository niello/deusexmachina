#include "RenderableAttribute.h"
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
#include <acl/math/vector4_32.h>

namespace Frame
{

void CRenderableAttribute::UpdateInSPS(Scene::CSPS& SPS)
{
	n_assert_dbg(IsActive());

	CAABB AABB;
	const bool IsAABBValid = GetLocalAABB(AABB);
	const bool SPSChanged = (pSPS != &SPS);

	if (pSPS && (SPSChanged || !IsAABBValid))
		pSPS->RemoveObject(ObjectHandle);

	if (!IsAABBValid)
	{
		// This object currently has no bounds at all, it can't be added to the level
		pSPS = nullptr;
		ObjectHandle = {};
	}
	else if (SPSChanged)
	{
		pSPS = &SPS;
		AABB.Transform(_pNode->GetWorldMatrix());
		ObjectHandle = SPS.AddObject(AABB, this);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
	else if (_pNode->GetTransformVersion() != LastTransformVersion) //!!! || LocalBox changed!
	{
		AABB.Transform(_pNode->GetWorldMatrix());
		SPS.UpdateObject(ObjectHandle, AABB);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
}
//---------------------------------------------------------------------

bool CRenderableAttribute::GetGlobalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!_pNode) FAIL;

	if (pSPS && _pNode->GetTransformVersion() == LastTransformVersion) //!!! && LocalBox not changed!
	{
		// TODO: use Center+Extents SIMD AABB everywhere?!
		const auto Center = ObjectHandle->second->BoxCenter;
		const auto Extent = ObjectHandle->second->BoxExtent;
		OutBox.Set(
			vector3(acl::vector_get_x(Center), acl::vector_get_y(Center), acl::vector_get_z(Center)),
			vector3(acl::vector_get_x(Extent), acl::vector_get_y(Extent), acl::vector_get_z(Extent)));
	}
	else
	{
		if (GetLocalAABB(OutBox, LOD)) FAIL;
		OutBox.Transform(_pNode->GetWorldMatrix());
	}

	OK;
}
//---------------------------------------------------------------------

void CRenderableAttribute::OnActivityChanged(bool Active)
{
	if (!Active && pSPS)
	{
		pSPS->RemoveObject(ObjectHandle);
		pSPS = nullptr;
		ObjectHandle = {};
	}
}
//---------------------------------------------------------------------

void CRenderableAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	CAABB AABB;
	if (GetGlobalAABB(AABB))
		DebugDraw.DrawBoxWireframe(AABB, Render::ColorRGBA(160, 220, 255, 255), 1.f);

	if (pSPS && ObjectHandle != pSPS->GetInvalidObjectHandle())
		DebugDraw.DrawBoxWireframe(pSPS->GetNodeAABB(ObjectHandle->second->NodeIndex, true), Render::ColorRGBA(160, 255, 160, 255), 1.f);
}
//---------------------------------------------------------------------

}
