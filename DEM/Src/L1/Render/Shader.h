#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/StringID.h>

// Shader resource object, created from a compiled shader binary and used
// by GPU as a part of CRenderState to configure programmable pipeline parts.

namespace Render
{

enum EConstType
{
	ConstType_Float	= 0,
	ConstType_Int,
	ConstType_Bool,

	ConstType_Other,

	ConstType_Invalid
};

class CShader: public Resources::CResourceObject
{
	//__DeclareClassNoFactory;

protected:

	EShaderType Type;

public:

	CShader(): Type(ShaderType_Invalid) {}

	virtual void			Destroy() = 0;

	virtual HConst			GetConstHandle(CStrID ID) const = 0;
	virtual HConstBuffer	GetConstBufferHandle(CStrID ID) const = 0;
	virtual HConstBuffer	GetConstBufferHandle(HConst hConst) const = 0;
	virtual HResource		GetResourceHandle(CStrID ID) const = 0;
	virtual HSampler		GetSamplerHandle(CStrID ID) const = 0;
	virtual EConstType		GetConstType(HConst hConst) const = 0;

	EShaderType				GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}

#endif
