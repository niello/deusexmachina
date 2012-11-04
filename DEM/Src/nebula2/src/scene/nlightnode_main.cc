#include "scene/nlightnode.h"
#include "gfx2/ngfxserver2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nLightNode, "nabstractshadernode");

bool nLightNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'THGL': // LGHT
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetType(nLight::StringToType(Value));
			OK;
		}
		case 'DHSC': // CSHD
		{
			SetCastShadows(DataReader.Read<bool>());
			OK;
		}
		default: return nAbstractShaderNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

// This sets the light state which is constant across all shape instances which are lit by this light.
const nLight& nLightNode::ApplyLight(nSceneServer*, nRenderContext*, const matrix44& lightTransform, const vector4& shadowLightMask)
{
	light.SetRange(shaderParams.GetArg(nShaderState::LightRange).GetFloat());
	light.SetDiffuse(shaderParams.GetArg(nShaderState::LightDiffuse).GetVector4());
	light.SetSpecular(shaderParams.GetArg(nShaderState::LightSpecular).GetVector4());
	light.SetAmbient(shaderParams.GetArg(nShaderState::LightAmbient).GetVector4());
	light.SetShadowLightMask(shadowLightMask);
	nGfxServer2::Instance()->AddLight(light, lightTransform);
	return light;
}
//---------------------------------------------------------------------

// This sets the lighting states which differ between rendering shape
// instances lit by this light. This method call should only be used
// when rendering with the lighting model "shader".
const nLight& nLightNode::RenderLight(nSceneServer*, nRenderContext*, const matrix44& lightTransform)
{
	nGfxServer2::Instance()->SetTransform(nGfxServer2::Light, lightTransform);
	nShader2* shd = nGfxServer2::Instance()->GetShader();

	// For directional lights, the light pos shader attributes actually hold the light direction
	if (shd->IsParameterUsed(nShaderState::ModelLightPos))
	{
		const matrix44& Tfm = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelLight);
		const vector3& Value = (light.GetType() == nLight::Directional) ? Tfm.z_component() : Tfm.pos_component();
		shd->SetVector3(nShaderState::ModelLightPos, Value);
	}
	return light;
}
//---------------------------------------------------------------------
