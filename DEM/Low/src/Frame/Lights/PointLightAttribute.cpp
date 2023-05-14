#include "PointLightAttribute.h"
#include <Math/AABB.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>

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

bool CPointLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	OutBox.Set(vector3::Zero, vector3(_Range, _Range, _Range));
	return true;
}
//---------------------------------------------------------------------

}
