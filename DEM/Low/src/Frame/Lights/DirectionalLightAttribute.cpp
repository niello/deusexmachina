#include "DirectionalLightAttribute.h"
#include <Render/AnalyticalLight.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CDirectionalLightAttribute, 'DLTA', Frame::CLightAttribute);

bool CDirectionalLightAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
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
			default: return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CDirectionalLightAttribute::Clone()
{
	PDirectionalLightAttribute ClonedAttr = n_new(CDirectionalLightAttribute());
	ClonedAttr->_Color = _Color;
	ClonedAttr->_Intensity = _Intensity;
	ClonedAttr->_CastsShadow = _CastsShadow;
	ClonedAttr->_DoOcclusionCulling = _DoOcclusionCulling; // Should be always false for omnipresent lights
	return ClonedAttr;
}
//---------------------------------------------------------------------

Render::PLight CDirectionalLightAttribute::CreateLight() const
{
	return std::make_unique<Render::CAnalyticalLight>(Render::ELightType::Directional);
}
//---------------------------------------------------------------------

void CDirectionalLightAttribute::UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const
{
	// Light sources that emit no light are considered invisible
	if (!(_Color & 0x00ffffff) || _Intensity <= 0.f)
	{
		Light.IsVisible = false;
		return;
	}

	auto pLight = static_cast<Render::CAnalyticalLight*>(&Light);
	pLight->GPUData.Color = vector3(Render::ColorGetRed(_Color), Render::ColorGetGreen(_Color), Render::ColorGetBlue(_Color)) * _Intensity;
	pLight->GPUData.InvDirection = Math::FromSIMD3(rtm::vector_normalize3(_pNode->GetWorldMatrix().z_axis));
}
//---------------------------------------------------------------------

bool CDirectionalLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	//???or simply return false and consider it as an omnipresent light?!
	OutBox = CAABB::Invalid;
	return true;
}
//---------------------------------------------------------------------

}
