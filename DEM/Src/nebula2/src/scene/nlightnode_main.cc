//------------------------------------------------------------------------------
//  nlightnode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/**
*/
bool
nLightNode::HasLight() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    This sets the light state which is constant across all shape
    instances which are lit by this light.
*/
const nLight&
nLightNode::ApplyLight(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& lightTransform, const vector4& shadowLightMask)
{
    this->light.SetTransform(lightTransform);
    this->light.SetRange(this->shaderParams.GetArg(nShaderState::LightRange).GetFloat());
    this->light.SetDiffuse(this->shaderParams.GetArg(nShaderState::LightDiffuse).GetVector4());
    this->light.SetSpecular(this->shaderParams.GetArg(nShaderState::LightSpecular).GetVector4());
    this->light.SetAmbient(this->shaderParams.GetArg(nShaderState::LightAmbient).GetVector4());
    this->light.SetShadowLightMask(shadowLightMask);
    nGfxServer2::Instance()->AddLight(this->light);
    return this->light;
}

//------------------------------------------------------------------------------
/**
    This sets the lighting states which differ between rendering shape
    instances lit by this light. This method call should only be used
    when rendering with the lighting model "shader".
*/
const nLight&
nLightNode::RenderLight(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& lightTransform)
{
    nGfxServer2::Instance()->SetTransform(nGfxServer2::Light, lightTransform);
    nShader2* shd = nGfxServer2::Instance()->GetShader();
    if (this->light.GetType() == nLight::Directional)
    {
        // for directional lights, the light pos shader attributes
        // actually hold the light direction
        if (shd->IsParameterUsed(nShaderState::ModelLightPos))
        {
            shd->SetVector3(nShaderState::ModelLightPos, nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelLight).z_component());
        }
    }
    else
    {
        // point light position
        if (shd->IsParameterUsed(nShaderState::ModelLightPos))
        {
            shd->SetVector3(nShaderState::ModelLightPos, nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelLight).pos_component());
        }
    }
    return this->light;
}
