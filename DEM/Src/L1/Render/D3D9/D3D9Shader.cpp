#include "D3D9Shader.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9Shader, 'SHD9', Render::CShader);

bool CD3D9Shader::Create(IUnknown* pShader)
{
	if (!pShader) FAIL;

	bool Result = false;
	IDirect3DVertexShader9* pVS = NULL;
	IDirect3DPixelShader9* pPS = NULL;
	if (SUCCEEDED(pShader->QueryInterface(&pVS)))
	{
		Result = Create(pVS);
		pVS->Release();
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pPS)))
	{
		Result = Create(pPS);
		pPS->Release();
	}

	return Result;
}
//---------------------------------------------------------------------

bool CD3D9Shader::Create(IDirect3DVertexShader9* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Vertex;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9Shader::Create(IDirect3DPixelShader9* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Pixel;
	OK;
}
//---------------------------------------------------------------------

void CD3D9Shader::InternalDestroy()
{
	SAFE_RELEASE(pD3DShader);

	Data::CHandleManager& HandleMgr = D3D9DrvFactory->HandleMgr;

	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		HHandle Handle = Consts[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Consts.Clear();

	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		HHandle Handle = Buffers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Buffers.Clear();

	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		HHandle Handle = Samplers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Samplers.Clear();
}
//---------------------------------------------------------------------

HConst CD3D9Shader::GetConstHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CD3D9ShaderConstMeta* pMeta = &Consts[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D9DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CD3D9Shader::GetConstBufferHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CD3D9ShaderBufferMeta* pMeta = &Buffers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D9DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}

	if (Buffers.GetCount())
	{
		// Default buffer
		CD3D9ShaderBufferMeta* pMeta = &Buffers[0];
		if (!pMeta->Handle) pMeta->Handle = D3D9DrvFactory->HandleMgr.OpenHandle(pMeta);
		return pMeta->Handle;
	}

	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CD3D9Shader::GetConstBufferHandle(HConst hConst) const
{
	if (!hConst) return INVALID_HANDLE;
	CD3D9ShaderConstMeta* pMeta = (CD3D9ShaderConstMeta*)D3D9DrvFactory->HandleMgr.GetHandleData(hConst);
	return pMeta ? pMeta->BufferHandle : INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CD3D9Shader::GetResourceHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CD3D9ShaderRsrcMeta* pMeta = &Resources[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D9DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HSampler CD3D9Shader::GetSamplerHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CD3D9ShaderSamplerMeta* pMeta = &Samplers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D9DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

}