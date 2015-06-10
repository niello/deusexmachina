//#pragma once
//#ifndef __DEM_L1_RENDER_D3D11_SHADER_VARS_H__
//#define __DEM_L1_RENDER_D3D11_SHADER_VARS_H__
//
//#include <Render/ShaderVars.h>
//
//// D3D11 implementation of shader vars. Can be used only with feature level 10 and above,
//// as it relies on assumption that a returned GPU memory pointer is 16-byte aligned.
//// D3D11Level9 implementation may be used with a feature level 9.
//
//namespace Render
//{
//
//class CD3D11ShaderVars: public CShaderVars
//{
//protected:
//
//	// Set of buffers (constant, texture), texture refs, sampler state refs
//
//	//???texture and sampler management here? here can be non-virtual!
//
//public:
//
//	//void	SetValue(HShaderParam Handle, const void* pData, DWORD Size);
//	//void	SetValueAligned16(HShaderParam Handle, const void* pData, DWORD Size);
//
//	virtual bool			BeginValues() = 0;
//	virtual bool			EndValues() = 0;
//	//???use __fastcall? for direct memory copy, mb it is good just to get address in a register, not on the stack?
//	virtual void			SetBool(HShaderParam Handle, const bool* pValues, DWORD Count) = 0;
//	virtual void			SetIntAsBool(HShaderParam Handle, const int* pValues, DWORD Count) = 0;
//	virtual void			SetInt(HShaderParam Handle, const int* pValues, DWORD Count) = 0;
//	virtual void			SetFloat(HShaderParam Handle, const float* pValues, DWORD Count) = 0;
//	virtual void			SetVector4(HShaderParam Handle, const vector4* pValues, DWORD Count) = 0;
//	virtual void			SetMatrix44(HShaderParam Handle, const matrix44* pValues, DWORD Count) = 0;
//	//???need? virtual void			SetMatrix44Transpose(HShaderParam Handle, const matrix44* pValues, DWORD Count) = 0;
//
//	const CShaderMetadata&	GetMetadata() const { return *Meta; }
//};
//
//}
//
//#endif
