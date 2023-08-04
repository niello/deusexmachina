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
	const bool SceneChanged = (_pScene != &Scene);

	if (_pScene && (SceneChanged || !IsAABBValid))
		_pScene->RemoveLight(_SceneRecordHandle);

	if (!IsAABBValid) //???need this branch or can use it for handling omnipresent lights like directional and global IBL?
	{
		// This light currently has no bounds at all, it can't be added to the level
		_pScene = nullptr;
		_SceneRecordHandle = {};
		return;
	}

	const bool BoundsChanged = (_pNode->GetTransformVersion() != _LastTransformVersion); //!!! || LocalBox changed!
	if (!SceneChanged && !BoundsChanged) return;

	// Neutralize scale, we can't use it for lights
	auto Tfm = _pNode->GetWorldMatrix();
	Tfm.AxisX().norm();
	Tfm.AxisY().norm();
	Tfm.AxisZ().norm();

	AABB.Transform(Tfm);

	const auto LocalSphere = GetLocalSphere();
	vector3 SpherePos(acl::vector_get_x(LocalSphere), acl::vector_get_y(LocalSphere), acl::vector_get_z(LocalSphere));
	SpherePos = Tfm.transform_coord(SpherePos);
	const auto GlobalSphere = acl::vector_set(SpherePos.x, SpherePos.y, SpherePos.z, acl::vector_get_w(LocalSphere));

	if (SceneChanged)
	{
		_pScene = &Scene;
		_SceneRecordHandle = Scene.AddLight(AABB, GlobalSphere, *this);
	}
	else // if (BoundsChanged)
	{
		// NB: spot light bounds are independent of rotation around dir axis, point - any rotation, dir - any tfm.
		// Could keep BoundsVersion unchanged in these cases. Worth it? To do this, need to compare prev and new light state, may be light dependent -> virtual call.
		Scene.UpdateLightBounds(_SceneRecordHandle, AABB, GlobalSphere);
	}

	_LastTransformVersion = _pNode->GetTransformVersion();
}
//---------------------------------------------------------------------

//!!!GetGlobalAABB & CalcBox must be separate!
bool CLightAttribute::GetGlobalAABB(CAABB& OutBox) const
{
	if (!_pNode) return false;

	if (_pScene && _pNode->GetTransformVersion() == _LastTransformVersion) //!!! && LocalBox not changed!
	{
		// TODO: use Center+Extents SIMD AABB everywhere?!
		const auto Center = _SceneRecordHandle->second.BoxCenter;
		const auto Extent = _SceneRecordHandle->second.BoxExtent;
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
	if (!Active && _pScene)
	{
		_pScene->RemoveLight(_SceneRecordHandle);
		_pScene = nullptr;
		_SceneRecordHandle = {};
	}
}
//---------------------------------------------------------------------

}
