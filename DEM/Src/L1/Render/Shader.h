#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Resources/ResourceObject.h>

// Shader resource object, created from a compiled shader binary and used
// by GPU as a part of CRenderState to configure programmable pipeline parts.

namespace Render
{

enum EShaderType
{
	ShaderType_Invalid,
	ShaderType_Vertex,
	ShaderType_Hull,
	ShaderType_Domain,
	ShaderType_Geometry,
	ShaderType_Pixel
};

class CShader: public Resources::CResourceObject
{
protected:

	EShaderType Type;

public:

	CShader(): Type(ShaderType_Invalid) {}

	virtual void	Destroy() = 0;
	EShaderType		GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}

#endif
