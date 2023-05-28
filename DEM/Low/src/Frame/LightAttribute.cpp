#include "LightAttribute.h"
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <acl/math/vector4_32.h>

namespace Frame
{

CLightAttribute::CLightAttribute()
	: _CastsShadow(false)
	, _DoOcclusionCulling(true)
{
}
//---------------------------------------------------------------------

void CLightAttribute::UpdateInGraphicsScene(CGraphicsScene& Scene)
{
	n_assert_dbg(IsActive());

	CAABB AABB;
	const bool IsAABBValid = GetLocalAABB(AABB);
	const bool SceneChanged = (pScene != &Scene);

	if (pScene && (SceneChanged || !IsAABBValid))
		pScene->RemoveLight(SceneRecordHandle);

	if (!IsAABBValid) //???need this branch or can use it for handling omnipresent lights like directional and global IBL?
	{
		// This light currently has no bounds at all, it can't be added to the level
		pScene = nullptr;
		SceneRecordHandle = {};
	}
	else if (SceneChanged)
	{
		pScene = &Scene;
		AABB.Transform(_pNode->GetWorldMatrix());
		SceneRecordHandle = Scene.AddLight(AABB, *this);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
	else if (_pNode->GetTransformVersion() != LastTransformVersion) //!!! || LocalBox changed!
	{
		AABB.Transform(_pNode->GetWorldMatrix());
		Scene.UpdateLightBounds(SceneRecordHandle, AABB);
		LastTransformVersion = _pNode->GetTransformVersion();
	}
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
bool CLightAttribute::GetGlobalAABB(CAABB& OutBox) const
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
		if (GetLocalAABB(OutBox)) return false;
		OutBox.Transform(_pNode->GetWorldMatrix());
	}

	return true;
}
//---------------------------------------------------------------------

void CLightAttribute::OnActivityChanged(bool Active)
{
	if (!Active && pScene)
	{
		pScene->RemoveLight(SceneRecordHandle);
		pScene = nullptr;
		SceneRecordHandle = {};
	}
}
//---------------------------------------------------------------------

}
