#include "IBLAmbientLightAttribute.h"
#include <Frame/GraphicsResourceManager.h>
#include <Render/ImageBasedLight.h>
#include <IO/BinaryReader.h>
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
	if (IsGlobal())
	{
		//???or simply return false and consider it as an omnipresent light?!
		OutBox.Set(vector3::Zero, vector3::Zero);
		return true;
	}
	else
	{
		OutBox.Set(vector3::Zero, vector3(_Range, _Range, _Range));
		return true;
	}
}
//---------------------------------------------------------------------

}
