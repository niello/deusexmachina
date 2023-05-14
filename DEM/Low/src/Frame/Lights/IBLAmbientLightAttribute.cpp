#include "IBLAmbientLightAttribute.h"
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CIBLAmbientLightAttribute, 'NAAL', Scene::CNodeAttribute);

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
	return ClonedAttr;
}
//---------------------------------------------------------------------

bool CIBLAmbientLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	// TODO: global IBL has no AABB, just like a directional light. Local IBL has cubic AABB like a point light.
	NOT_IMPLEMENTED;
	FAIL;
}
//---------------------------------------------------------------------

}
