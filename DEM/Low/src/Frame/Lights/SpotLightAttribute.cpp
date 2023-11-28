#include "SpotLightAttribute.h"
#include <Render/AnalyticalLight.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <Math/CameraMath.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>
#include <Debug/DebugDraw.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CSpotLightAttribute, 'SLTA', Frame::CLightAttribute);

bool CSpotLightAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'CSHD':
			{
				_CastsShadow = DataReader.Read<bool>();
				break;
			}
			//case '????': //!!!TODO!
			//{
			//	_DoOcclusionCulling = DataReader.Read<bool>();
			//	break;
			//}
			case 'LCLR':
			{
				DataReader.Read(_Color);
				break;
			}
			case 'LINT':
			{
				DataReader.Read(_Intensity);
				break;
			}
			case 'LRNG':
			{
				DataReader.Read(_Range);
				break;
			}
			case 'LCIN':
			{
				_ConeInner = n_deg2rad(DataReader.Read<float>());
				break;
			}
			case 'LCOU':
			{
				_ConeOuter = n_deg2rad(DataReader.Read<float>());
				break;
			}
			default: return false;
		}
	}

	// Clamp angles to valid values
	_ConeInner = std::clamp(_ConeInner, 0.f, PI);
	_ConeOuter = std::clamp(_ConeOuter, _ConeInner, PI);

	_CosHalfInner = rtm::scalar_cos(0.5f * _ConeInner);

	// Calculate bounding sphere of a cone. Use local offset along the look axis for the sphere center.
	// See: https://bartwronski.com/2017/04/13/cull-that-cone/
	rtm::scalar_sincos(0.5f * _ConeOuter, _SinHalfOuter, _CosHalfOuter);
	if (_CosHalfOuter < COS_PI_DIV_4)
	{
		_BoundingSphereOffsetAlongDir = _Range * _CosHalfOuter;
		_BoundingSphereRadius = _Range * _SinHalfOuter;
	}
	else
	{
		_BoundingSphereOffsetAlongDir = _Range / (2.0f * _CosHalfOuter);
		_BoundingSphereRadius = _BoundingSphereOffsetAlongDir;
	}

	return true;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSpotLightAttribute::Clone()
{
	PSpotLightAttribute ClonedAttr = n_new(CSpotLightAttribute());
	ClonedAttr->_BoundingSphereOffsetAlongDir = _BoundingSphereOffsetAlongDir;
	ClonedAttr->_BoundingSphereRadius = _BoundingSphereRadius;
	ClonedAttr->_Color = _Color;
	ClonedAttr->_Intensity = _Intensity;
	ClonedAttr->_Range = _Range;
	ClonedAttr->_ConeInner = _ConeInner;
	ClonedAttr->_ConeOuter = _ConeOuter;
	ClonedAttr->_SinHalfOuter = _SinHalfOuter;
	ClonedAttr->_CosHalfOuter = _CosHalfOuter;
	ClonedAttr->_CosHalfInner = _CosHalfInner;
	ClonedAttr->_CastsShadow = _CastsShadow;
	ClonedAttr->_DoOcclusionCulling = _DoOcclusionCulling;
	return ClonedAttr;
}
//---------------------------------------------------------------------

Render::PLight CSpotLightAttribute::CreateLight() const
{
	return std::make_unique<Render::CAnalyticalLight>(Render::ELightType::Spot);
}
//---------------------------------------------------------------------

void CSpotLightAttribute::UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const
{
	// Light sources that emit no light are considered invisible
	if (!(_Color & 0x00ffffff) || _Intensity <= 0.f)
	{
		Light.IsVisible = false;
		return;
	}

	auto pLight = static_cast<Render::CAnalyticalLight*>(&Light);
	pLight->GPUData.Color = vector3(Render::ColorGetRed(_Color), Render::ColorGetGreen(_Color), Render::ColorGetBlue(_Color)) * _Intensity;
	pLight->GPUData.Position = Math::FromSIMD3(_pNode->GetWorldPosition());
	pLight->GPUData.InvDirection = Math::FromSIMD3(rtm::vector_normalize3(_pNode->GetWorldMatrix().z_axis));
	pLight->GPUData.SqInvRange = 1.f / (_Range * _Range);
	pLight->GPUData.Params.x = _CosHalfInner;
	pLight->GPUData.Params.y = _CosHalfOuter;
}
//---------------------------------------------------------------------

bool CSpotLightAttribute::GetLocalAABB(Math::CAABB& OutBox) const
{
	const float HalfFarExtent = _Range * _SinHalfOuter / _CosHalfOuter;
	OutBox.Center = rtm::vector_set(0.f, 0.f, -0.5f * _Range);
	OutBox.Extent = rtm::vector_set(HalfFarExtent, HalfFarExtent, 0.5f * _Range);
	return true;
}
//---------------------------------------------------------------------

bool CSpotLightAttribute::IntersectsWith(rtm::vector4f_arg0 Sphere) const
{
	const rtm::vector4f LightDir = rtm::vector_normalize3(rtm::vector_neg(_pNode->GetWorldMatrix().z_axis));

	const float SphereRadius = rtm::vector_get_w(Sphere);

	// Check the bounding sphere of the light first
	const rtm::vector4f BoundingSpherePos = rtm::vector_mul_add(LightDir, _BoundingSphereOffsetAlongDir, _pNode->GetWorldPosition());
	const float TotalRadius = SphereRadius + _BoundingSphereRadius;
	if (rtm::vector_length_squared3(rtm::vector_sub(BoundingSpherePos, Sphere)) > TotalRadius * TotalRadius) return false;

	// Check sphere-cone intersection

	rtm::vector4f DistanceVector = rtm::vector_sub(Sphere, rtm::vector_mul(LightDir, (SphereRadius / _SinHalfOuter)));
	float SqDistance = rtm::vector_length_squared3(DistanceVector);
	float ProjectedLength = rtm::vector_dot3(LightDir, DistanceVector);
	if (ProjectedLength <= 0.f || ProjectedLength * ProjectedLength < SqDistance * _CosHalfOuter * _CosHalfOuter) return false;

	DistanceVector = rtm::vector_sub(Sphere, _pNode->GetWorldPosition());
	SqDistance = rtm::vector_length_squared3(DistanceVector);
	ProjectedLength = -rtm::vector_dot3(LightDir, DistanceVector);
	return ProjectedLength <= 0.f || ProjectedLength * ProjectedLength < SqDistance * _SinHalfOuter * _SinHalfOuter || SqDistance <= SphereRadius * SphereRadius;
}
//---------------------------------------------------------------------

U8 CSpotLightAttribute::TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const
{
	// Check the bounding sphere of the light first
	if (!Math::HasIntersection(rtm::vector_set_w(_pNode->GetWorldPosition(), _Range), BoxCenter, BoxExtent)) return Math::ClipOutside;

	// Update cached world space frustum
	if (_WorldBoundsCacheVersion != _pNode->GetTransformVersion())
	{
		const rtm::matrix4x4f LocalFrustum = Math::matrix_perspective_rh(_ConeOuter, 1.f, 0.001f, _Range);
		const rtm::matrix4x4f GlobalFrustum = rtm::matrix_mul(rtm::matrix_cast(rtm::matrix_inverse(_pNode->GetWorldMatrix())), LocalFrustum);
		_WorldFrustum = Math::CalcFrustumParams(GlobalFrustum);
		_WorldBoundsCacheVersion = _pNode->GetTransformVersion();
	}

	// Test AABB against the bounding frustum of the spot light
	return Math::ClipAABB(BoxCenter, BoxExtent, _WorldFrustum);
}
//---------------------------------------------------------------------

void CSpotLightAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	// FIXME: reuse _WorldFrustum
	const rtm::matrix4x4f LocalFrustum = Math::matrix_perspective_rh(_ConeOuter, 1.f, 0.001f, _Range);
	const rtm::matrix4x4f GlobalFrustum = rtm::matrix_mul(rtm::matrix_cast(rtm::matrix_inverse(_pNode->GetWorldMatrix())), LocalFrustum);
	DebugDraw.DrawFrustumWireframe(GlobalFrustum, Render::ColorRGBA(255, 255, 0, 255));
}
//---------------------------------------------------------------------

}
