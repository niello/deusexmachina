#pragma once
#ifndef __DEM_L1_RENDER_D3D9_SHADER_H__
#define __DEM_L1_RENDER_D3D9_SHADER_H__

#include <Render/Shader.h>
#include <Render/D3D9/D3D9Fwd.h>

// Direct3D9 shader object implementation

struct IUnknown;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

namespace Render
{

class CD3D9Shader: public CShader
{
	__DeclareClass(CD3D9Shader);

protected:

	IUnknown*	pD3DShader;

	void		InternalDestroy();

public:

	CFixedArray<CD3D9ShaderBufferMeta>	Buffers;
	CFixedArray<CD3D9ShaderConstMeta>	Consts;
	CFixedArray<CD3D9ShaderRsrcMeta>	Resources;
	CFixedArray<CD3D9ShaderSamplerMeta>	Samplers;

	CD3D9Shader(): pD3DShader(NULL) {}
	virtual ~CD3D9Shader() { InternalDestroy(); }

	bool					Create(IUnknown* pShader); 
	bool					Create(IDirect3DVertexShader9* pShader);
	bool					Create(IDirect3DPixelShader9* pShader);
	virtual void			Destroy() { InternalDestroy(); }

	virtual bool			IsResourceValid() const { return !!pD3DShader; }

	virtual HConst			GetConstHandle(CStrID ID) const;
	virtual HConstBuffer	GetConstBufferHandle(CStrID ID) const;
	virtual HConstBuffer	GetConstBufferHandle(HConst hConst) const;
	virtual HResource		GetResourceHandle(CStrID ID) const;
	virtual HSampler		GetSamplerHandle(CStrID ID) const;

	IUnknown*				GetD3DShader() const { return pD3DShader; }
	IDirect3DVertexShader9*	GetD3DVertexShader() const { n_assert_dbg(Type == ShaderType_Vertex); return (IDirect3DVertexShader9*)pD3DShader; }
	IDirect3DPixelShader9*	GetD3DPixelShader() const { n_assert_dbg(Type == ShaderType_Pixel); return (IDirect3DPixelShader9*)pD3DShader; }
};

typedef Ptr<CD3D9Shader> PD3D9Shader;

}

#endif
