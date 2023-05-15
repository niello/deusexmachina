#include "SpotLightAttribute.h"
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

	return true;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CSpotLightAttribute::Clone()
{
	PSpotLightAttribute ClonedAttr = n_new(CSpotLightAttribute());
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

Render::PLight CSpotLightAttribute::CreateLight(CGraphicsResourceManager& ResMgr) const
{
	//Ptr<Render::CSpotLight> Light = n_new(Render::CSpotLight());
	//Light->... = ...;
	//return Light;
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

}
