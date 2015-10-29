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

//enum EShaderParamType
//{
//	ShaderParam_ConstantFloat,
//	ShaderParam_ConstantInt,
//	ShaderParam_ConstantBool,
//	ShaderParam_CBuffer,
//	ShaderParam_Resource,
//	ShaderParam_Sampler
//};

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
	virtual HResource		GetResourceHandle(CStrID ID) const = 0;
	virtual HSampler		GetSamplerHandle(CStrID ID) const = 0;
	//virtual DWORD		GetConstAddress(CStrID ID, HConstBuffer& OutHostBuffer) = 0;

	EShaderType				GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}

#endif
