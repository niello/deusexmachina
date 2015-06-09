#pragma once
#ifndef __DEM_L1_RENDER_D3D11_SHADER_VARS_H__
#define __DEM_L1_RENDER_D3D11_SHADER_VARS_H__

#include <Render/ShaderVars.h>

// D3D11 implementation of shader vars. Can be used only with feature level 10 and above,
// as it relies on assumption that a returned GPU memory pointer is 16-byte aligned.
// D3D11Level9 implementation may be used with a feature level 9.

namespace Render
{

//???decouple constant buffer class from a wider ShaderVars class?
class CD3D11ShaderVars: public CShaderVars
{
protected:

	enum
	{
		InBeginValues = 0x01,
		BufferMapped = 0x02
	};
	// Set of buffers (constant, texture), texture refs, sampler state refs

	//???texture and sampler management here? here can be non-virtual!

public:

	virtual bool	BeginValues();
	virtual bool	EndValues();
	virtual void	SetBool(HShaderParam Handle, const bool* pValues, DWORD Count);
	virtual void	SetIntAsBool(HShaderParam Handle, const int* pValues, DWORD Count);
	virtual void	SetInt(HShaderParam Handle, const int* pValues, DWORD Count);
	virtual void	SetFloat(HShaderParam Handle, const float* pValues, DWORD Count);
	virtual void	SetVector4(HShaderParam Handle, const vector4* pValues, DWORD Count);
	virtual void	SetMatrix44(HShaderParam Handle, const matrix44* pValues, DWORD Count);

	void			SetValue(HShaderParam Handle, const void* pData, DWORD Size);
	void			SetValueAligned16(HShaderParam Handle, const void* pData, DWORD Size);
};

}

#endif
