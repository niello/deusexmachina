#include "D3D11Shader.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11Shader, 'SHD1', Render::CShader);

bool CD3D11Shader::Create(ID3D11DeviceChild* pShader)
{
	if (!pShader) FAIL;

	bool Result = false;
	ID3D11VertexShader* pVS = NULL;
	ID3D11HullShader* pHS = NULL;
	ID3D11DomainShader* pDS = NULL;
	ID3D11GeometryShader* pGS = NULL;
	ID3D11PixelShader* pPS = NULL;
	if (SUCCEEDED(pShader->QueryInterface(&pVS)))
	{
		Result = Create(pVS);
		pVS->Release();
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pHS)))
	{
		Result = Create(pHS);
		pHS->Release();
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pDS)))
	{
		Result = Create(pDS);
		pDS->Release();
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pGS)))
	{
		Result = Create(pGS);
		pGS->Release();
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pPS)))
	{
		Result = Create(pPS);
		pPS->Release();
	}

	return Result;
}
//---------------------------------------------------------------------

bool CD3D11Shader::Create(ID3D11VertexShader* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Vertex;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Shader::Create(ID3D11HullShader* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Hull;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Shader::Create(ID3D11DomainShader* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Domain;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Shader::Create(ID3D11GeometryShader* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Geometry;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Shader::Create(ID3D11PixelShader* pShader)
{
	if (!pShader) FAIL;
	pD3DShader = pShader;
	Type = ShaderType_Pixel;
	OK;
}
//---------------------------------------------------------------------

void CD3D11Shader::InternalDestroy()
{
	SAFE_RELEASE(pD3DShader);

	Data::CHandleManager& HandleMgr = D3D11DrvFactory->HandleMgr;

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

	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		HHandle Handle = Resources[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Resources.Clear();

	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		HHandle Handle = Samplers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Samplers.Clear();
}
//---------------------------------------------------------------------

HConst CD3D11Shader::GetConstHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CConstMeta* pMeta = &Consts[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D11DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CD3D11Shader::GetConstBufferHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CBufferMeta* pMeta = &Buffers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D11DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CD3D11Shader::GetResourceHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CRsrcMeta* pMeta = &Resources[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D11DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HSampler CD3D11Shader::GetSamplerHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CRsrcMeta* pMeta = &Samplers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = D3D11DrvFactory->HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

}