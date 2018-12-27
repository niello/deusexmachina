#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/StringID.h>

// Shader resource object, created from a compiled shader (HLSL etc) program and used
// by GPU as a part of CRenderState to configure programmable pipeline parts.

namespace Render
{
class IShaderMetadata;

class CShader: public Resources::CResourceObject
{
	//__DeclareClassNoFactory;

protected:

	EShaderType Type;

public:

	CShader(): Type(ShaderType_Invalid) {}

	virtual void					Destroy() = 0;
	virtual const IShaderMetadata*	GetMetadata() const = 0;
	EShaderType						GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}

#endif
