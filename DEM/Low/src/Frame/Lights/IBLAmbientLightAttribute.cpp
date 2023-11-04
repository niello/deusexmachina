#include "IBLAmbientLightAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Render/ImageBasedLight.h>
#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
#include <Math/CameraMath.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CIBLAmbientLightAttribute, 'NAAL', Frame::CLightAttribute);

bool CIBLAmbientLightAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'IRRM':
			{
				_IrradianceMapUID = CStrID(DataReader.Read<CString>());
				break;
			}
			case 'PMRM':
			{
				_RadianceEnvMapUID = CStrID(DataReader.Read<CString>());
				break;
			}
			case 'LRNG':
			{
				DataReader.Read(_Range);
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CIBLAmbientLightAttribute::Clone()
{
	PIBLAmbientLightAttribute ClonedAttr = n_new(CIBLAmbientLightAttribute());
	ClonedAttr->_IrradianceMapUID = _IrradianceMapUID;
	ClonedAttr->_RadianceEnvMapUID = _RadianceEnvMapUID;
	ClonedAttr->_Range = _Range;
	ClonedAttr->_CastsShadow = _CastsShadow;
	ClonedAttr->_DoOcclusionCulling = _DoOcclusionCulling;
	return ClonedAttr;
}
//---------------------------------------------------------------------

Render::PLight CIBLAmbientLightAttribute::CreateLight() const
{
	return std::make_unique<Render::CImageBasedLight>();
}
//---------------------------------------------------------------------

void CIBLAmbientLightAttribute::UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const
{
	// Light sources that emit no light are considered invisible
	if (!_IrradianceMapUID && !_RadianceEnvMapUID)
	{
		Light.IsVisible = false;
		return;
	}

	//!!!FIXME: don't search IBL textures each frame, use cache until UID changes! Update only when nullptr? Or store UID in GPU resource / CTextureData for comparison?
	auto pLight = static_cast<Render::CImageBasedLight*>(&Light);
	pLight->_IrradianceMap = ResMgr.GetTexture(_IrradianceMapUID, Render::Access_GPU_Read);
	pLight->_RadianceEnvMap = ResMgr.GetTexture(_RadianceEnvMapUID, Render::Access_GPU_Read);

	if (!IsGlobal())
	{
		// update bounds and mark them dirty
	}
}
//---------------------------------------------------------------------

bool CIBLAmbientLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	// Negative range is treated as invalid bounds which is desired for global IBL source
	OutBox.Set(vector3::Zero, vector3(_Range, _Range, _Range));
	return true;
}
//---------------------------------------------------------------------

bool CIBLAmbientLightAttribute::IntersectsWith(acl::Vector4_32Arg0 Sphere) const
{
	if (IsGlobal()) return true;

	const auto& Pos = _pNode->GetWorldPosition();
	const acl::Vector4_32 LightPos = acl::vector_set(Pos.x, Pos.y, Pos.z);

	const float TotalRadius = acl::vector_get_w(Sphere) + _Range;

	return acl::vector_length_squared3(acl::vector_sub(LightPos, Sphere)) <= TotalRadius * TotalRadius;
}
//---------------------------------------------------------------------

U8 CIBLAmbientLightAttribute::TestBoxClipping(acl::Vector4_32Arg0 BoxCenter, acl::Vector4_32Arg1 BoxExtent) const
{
	if (IsGlobal()) return Math::ClipInside;
	const auto& Pos = _pNode->GetWorldPosition();
	return Math::ClipAABB(BoxCenter, BoxExtent, acl::vector_set(Pos.x, Pos.y, Pos.z, _Range));
}
//---------------------------------------------------------------------

}
