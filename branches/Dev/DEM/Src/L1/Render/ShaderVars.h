#pragma once
#ifndef __DEM_L1_RENDER_SHADER_VARS_H__
#define __DEM_L1_RENDER_SHADER_VARS_H__

#include <Render/ShaderMetadata.h>

// An instance of a shader variable data. Manages passing constants, textures, buffers
// and sampler states to the GPU. Use contained metadata to obtain variable handles.

namespace Render
{

class CShaderVars //???CObject? or at least CRefCouned?
{
protected:

	PShaderMetadata Meta;

	// Set of buffers (constant, texture), texture refs, sampler state refs

	CShaderVars(CShaderMetadata* Metadata): Meta(Metadata) {}

	friend class CShaderMetadata;

public:

	//void	SetValue(HShaderVar Handle, const void* pData, DWORD Size);
	//void	SetValueAligned16(HShaderVar Handle, const void* pData, DWORD Size);

	virtual bool			BeginValues() = 0;
	virtual bool			EndValues() = 0;
	virtual void			SetBool(HShaderVar Handle, const bool* pValues, DWORD Count) = 0;
	virtual void			SetIntAsBool(HShaderVar Handle, const int* pValues, DWORD Count) = 0;
	virtual void			SetInt(HShaderVar Handle, const int* pValues, DWORD Count) = 0;
	virtual void			SetFloat(HShaderVar Handle, const float* pValues, DWORD Count) = 0;
	virtual void			SetVector4(HShaderVar Handle, const vector4* pValues, DWORD Count) = 0;
	virtual void			SetMatrix44(HShaderVar Handle, const matrix44* pValues, DWORD Count) = 0;

	const CShaderMetadata&	GetMetadata() const { return *Meta; }
};

}

#endif
