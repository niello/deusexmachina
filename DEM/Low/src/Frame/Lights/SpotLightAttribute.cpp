#include "SpotLightAttribute.h"
#include <Render/SpotLight.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>

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

	// Calculate bounding sphere of a cone. Use local offset along the look axis for the sphere center.
	// See: https://bartwronski.com/2017/04/13/cull-that-cone/
	acl::sincos(_ConeOuter * 0.5f, _SinHalfOuter, _CosHalfOuter);
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
	ClonedAttr->_CastsShadow = _CastsShadow;
	ClonedAttr->_DoOcclusionCulling = _DoOcclusionCulling;
	return ClonedAttr;
}
//---------------------------------------------------------------------

Render::PLight CSpotLightAttribute::CreateLight() const
{
	return std::make_unique<Render::CSpotLight>();
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

	//auto pLight = static_cast<Render::CSpotLight*>(&Light);
	//if (_pNode) pLight->SetDirection(-_pNode->GetWorldMatrix().AxisZ());
}
//---------------------------------------------------------------------

bool CSpotLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	//!!!can cache HalfFarExtent!
	const float HalfFarExtent = _Range * n_tan(_ConeOuter * 0.5f);
	OutBox.Min.set(-HalfFarExtent, -HalfFarExtent, -_Range);
	OutBox.Max.set(HalfFarExtent, HalfFarExtent, 0.f);
	return true;
}
//---------------------------------------------------------------------

bool CSpotLightAttribute::IntersectsWith(acl::Vector4_32Arg0 Sphere) const
{
	const auto& Pos = _pNode->GetWorldPosition();
	const acl::Vector4_32 LightPos = acl::vector_set(Pos.x, Pos.y, Pos.z);

	const auto AxisZ = _pNode->GetWorldMatrix().AxisZ();
	const acl::Vector4_32 LightDir = acl::vector_normalize3(acl::vector_set(-AxisZ.x, -AxisZ.y, -AxisZ.z));

	const float SphereRadius = acl::vector_get_w(Sphere);

	// Check the bounding sphere of the light first
	const acl::Vector4_32 BoundingSpherePos = acl::vector_mul_add(LightDir, _BoundingSphereOffsetAlongDir, LightPos);
	const float TotalRadius = SphereRadius + _BoundingSphereRadius;
	if (acl::vector_length_squared3(acl::vector_sub(BoundingSpherePos, Sphere)) > TotalRadius * TotalRadius) return false;

	// Check sphere-cone intersection

	acl::Vector4_32 DistanceVector = acl::vector_sub(Sphere, acl::vector_mul(LightDir, (SphereRadius / _SinHalfOuter)));
	float SqDistance = acl::vector_length_squared3(DistanceVector);
	float ProjectedLength = acl::vector_dot3(LightDir, DistanceVector);
	if (ProjectedLength <= 0.f || ProjectedLength * ProjectedLength < SqDistance * _CosHalfOuter * _CosHalfOuter) return false;

	DistanceVector = acl::vector_sub(Sphere, LightPos);
	SqDistance = acl::vector_length_squared3(DistanceVector);
	ProjectedLength = -acl::vector_dot3(LightDir, DistanceVector);
	return ProjectedLength <= 0.f || ProjectedLength * ProjectedLength < SqDistance * _SinHalfOuter * _SinHalfOuter || SqDistance <= SphereRadius * SphereRadius;
}
//---------------------------------------------------------------------

}
