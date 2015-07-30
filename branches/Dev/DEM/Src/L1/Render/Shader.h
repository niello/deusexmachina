#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Resources/ResourceObject.h>
#include <Data/StringID.h>

// Shader resource object, created from a compiled shader binary and used
// by GPU as a part of CRenderState to configure programmable pipeline parts.

namespace Render
{
// Binding handlers for shader parameters
typedef DWORD HConstBuffer;
typedef DWORD HResource;
typedef DWORD HSampler;

enum EShaderType
{
	ShaderType_Invalid,
	ShaderType_Vertex,
	ShaderType_Hull,
	ShaderType_Domain,
	ShaderType_Geometry,
	ShaderType_Pixel
};

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

	virtual HConstBuffer	GetConstBufferHandle(CStrID ID) const = 0;
	virtual HResource		GetResourceHandle(CStrID ID) const = 0;
	virtual HSampler		GetSamplerHandle(CStrID ID) const = 0;
	//virtual DWORD		GetConstAddress(CStrID ID, HConstBuffer& OutHostBuffer) = 0;

	EShaderType				GetType() const { return Type; }
};

typedef Ptr<CShader> PShader;

}

#endif
