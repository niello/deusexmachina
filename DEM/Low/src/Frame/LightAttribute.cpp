#include "LightAttribute.h"
#include <Scene/SceneNode.h>
#include <Math/SIMDMath.h>

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

	Math::CAABB AABB;
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
	const rtm::matrix3x4f Tfm = rtm::matrix_remove_scale(_pNode->GetWorldMatrix());

	AABB = Math::AABBFromOBB(AABB, Tfm);

	const auto LocalSphere = GetLocalSphere();
	const rtm::vector4f SpherePos = rtm::matrix_mul_point3(LocalSphere, Tfm);
	const auto GlobalSphere = Math::vector_mix_xyzd(SpherePos, LocalSphere); // World xyz + radius

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

bool CLightAttribute::GetGlobalAABB(Math::CAABB& OutBox) const
{
	if (!_pNode) return false;

	if (_pScene && _pNode->GetTransformVersion() == _LastTransformVersion) //!!! && LocalBox not changed!
	{
		OutBox = _SceneRecordHandle->second.Box;
	}
	else
	{
		if (GetLocalAABB(OutBox)) return false;
		OutBox = Math::AABBFromOBB(OutBox, _pNode->GetWorldMatrix());
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
