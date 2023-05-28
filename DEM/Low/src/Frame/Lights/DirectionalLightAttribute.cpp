#include "DirectionalLightAttribute.h"
#include <Render/DirectionalLight.h>
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
	return std::make_unique<Render::CDirectionalLight>();
}
//---------------------------------------------------------------------

void CDirectionalLightAttribute::UpdateLight(CGraphicsResourceManager& ResMgr, Render::CLight& Light) const
{
	auto pLight = static_cast<Render::CDirectionalLight*>(&Light);
	if (_pNode) pLight->SetDirection(-_pNode->GetWorldMatrix().AxisZ());
	//pLight->Color
	//pLight->Intensity
}
//---------------------------------------------------------------------

bool CDirectionalLightAttribute::GetLocalAABB(CAABB& OutBox) const
{
	//???or simply return false and consider it as an omnipresent light?!
	OutBox.Set(vector3::Zero, vector3::Zero);
	return true;
}
//---------------------------------------------------------------------

}
