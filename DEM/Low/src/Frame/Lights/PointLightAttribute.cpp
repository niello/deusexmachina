#include "PointLightAttribute.h"
#include <Render/AnalyticalLight.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <Math/CameraMath.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>
#include <Debug/DebugDraw.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CPointLightAttribute, 'PLTA', Frame::CLightAttribute);

bool CPointLightAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
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
			default: return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CPointLightAttribute::Clone()
{
	PPointLightAttribute ClonedAttr = n_new(CPointLightAttribute());
	ClonedAttr->_Color = _Color;
	ClonedAttr->_Intensity = _Intensity;
	ClonedAttr->_Range = _Range;
	ClonedAttr->_CastsShadow = _CastsShadow;
	ClonedAttr->_DoOcclusionCulling = _DoOcclusionCulling;
	return ClonedAttr;
}
//---------------------------------------------------------------------

Render::PLight CPointLightAttribute::CreateLight() const
{
	return std::make_unique<Render::CAnalyticalLight>(Render::ELightType::Point);
}
//---------------------------------------------------------------------

void CPointLightAttribute::UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const
{
	// Light sources that emit no light are considered invisible
	if (!(_Color & 0x00ffffff) || _Intensity <= 0.f)
	{
		Light.IsVisible = false;
		return;
	}

	Light.GPUData.Color = vector3(Render::ColorGetRed(_Color), Render::ColorGetGreen(_Color), Render::ColorGetBlue(_Color)) * _Intensity * 0.01f;
	Light.GPUData.Position = _pNode->GetWorldMatrix().Translation();
	Light.GPUData.SqInvRange = 1.f / (_Range * _Range);
}
//---------------------------------------------------------------------

bool CPointLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	OutBox.Set(vector3::Zero, vector3(_Range, _Range, _Range));
	return true;
}
//---------------------------------------------------------------------

bool CPointLightAttribute::IntersectsWith(acl::Vector4_32Arg0 Sphere) const
{
	const auto& Pos = _pNode->GetWorldPosition();
	const acl::Vector4_32 LightPos = acl::vector_set(Pos.x, Pos.y, Pos.z);

	const float TotalRadius = acl::vector_get_w(Sphere) + _Range;

	return acl::vector_length_squared3(acl::vector_sub(LightPos, Sphere)) <= TotalRadius * TotalRadius;
}
//---------------------------------------------------------------------

U8 CPointLightAttribute::TestBoxClipping(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const
{
	const auto& Pos = _pNode->GetWorldPosition();
	return Math::ClipAABB(BoxCenter, BoxExtent, acl::vector_set(Pos.x, Pos.y, Pos.z, _Range));
}
//---------------------------------------------------------------------

void CPointLightAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	DebugDraw.DrawSphereWireframe(_pNode->GetWorldPosition(), _Range, Render::ColorRGBA(255, 255, 0, 255));
}
//---------------------------------------------------------------------

}
