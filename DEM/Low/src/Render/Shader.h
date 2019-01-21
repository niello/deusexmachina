#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>

// Shader resource object, created from a compiled shader (HLSL etc) program and used
// by GPU as a part of CRenderState to configure programmable pipeline parts.

namespace Render
{
class IShaderMetadata;

class CShader: public Core::CObject
{
protected:

	EShaderType Type = ShaderType_Invalid;

public:

	virtual void					Destroy() = 0;
	virtual bool					IsValid() const = 0;
	virtual const IShaderMetadata*	GetMetadata() const = 0;
	EShaderType						GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}
