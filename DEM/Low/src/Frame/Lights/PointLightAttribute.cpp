#include "PointLightAttribute.h"
#include <Render/PointLight.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>
#include <Core/Factory.h>
#include <IO/BinaryReader.h>
#include <acl/math/vector4_32.h>

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
	return std::make_unique<Render::CPointLight>();
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

	//auto pLight = static_cast<Render::CPointLight*>(&Light);
	// update pos
	//???apply scale?
}
//---------------------------------------------------------------------

bool CPointLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	OutBox.Set(vector3::Zero, vector3(_Range, _Range, _Range));
	return true;
}
//---------------------------------------------------------------------

bool CPointLightAttribute::IntersectsWith(acl::Vector4_32 SphereCenter, float SphereRadius) const
{
	const auto& Pos = _pNode->GetWorldPosition();
	const acl::Vector4_32 LightPos = acl::vector_set(Pos.x, Pos.y, Pos.z);

	const float TotalRadius = SphereRadius + _Range;

	return acl::vector_length_squared3(acl::vector_sub(LightPos, SphereCenter)) <= TotalRadius * TotalRadius;
}
//---------------------------------------------------------------------

}
