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

	Light.GPUData.Color = vector3(Render::ColorGetRed(_Color), Render::ColorGetGreen(_Color), Render::ColorGetBlue(_Color)) * (_Intensity / 255.f);
	Light.GPUData.Position = Math::FromSIMD3(_pNode->GetWorldPosition());
	Light.GPUData.SqInvRange = 1.f / (_Range * _Range);
}
//---------------------------------------------------------------------

bool CPointLightAttribute::GetLocalAABB(Math::CAABB& OutBox) const
{
	OutBox.Center = rtm::vector_zero();
	OutBox.Extent = rtm::vector_set(_Range);
	return true;
}
//---------------------------------------------------------------------

bool CPointLightAttribute::IntersectsWith(rtm::vector4f_arg0 Sphere) const
{
	const float TotalRadius = rtm::vector_get_w(Sphere) + _Range;
	return rtm::vector_length_squared3(rtm::vector_sub(_pNode->GetWorldPosition(), Sphere)) <= TotalRadius * TotalRadius;
}
//---------------------------------------------------------------------

U8 CPointLightAttribute::TestBoxClipping(rtm::vector4f_arg0 BoxCenter, rtm::vector4f_arg1 BoxExtent) const
{
	return Math::ClipAABB(BoxCenter, BoxExtent, rtm::vector_set_w(_pNode->GetWorldPosition(), _Range));
}
//---------------------------------------------------------------------

void CPointLightAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	DebugDraw.DrawSphereWireframe(Math::FromSIMD3(_pNode->GetWorldPosition()), _Range, Render::ColorRGBA(255, 255, 0, 255));
}
//---------------------------------------------------------------------

}
