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
	ID3D11VertexShader* pVS = nullptr;
	ID3D11HullShader* pHS = nullptr;
	ID3D11DomainShader* pDS = nullptr;
	ID3D11GeometryShader* pGS = nullptr;
	ID3D11PixelShader* pPS = nullptr;
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
	Metadata.Clear();
}
//---------------------------------------------------------------------

ID3D11VertexShader* CD3D11Shader::GetD3DVertexShader() const
{
	n_assert_dbg(Type == ShaderType_Vertex);
	return static_cast<ID3D11VertexShader*>(pD3DShader);
}
//---------------------------------------------------------------------

ID3D11HullShader* CD3D11Shader::GetD3DHullShader() const
{
	n_assert_dbg(Type == ShaderType_Hull);
	return static_cast<ID3D11HullShader*>(pD3DShader);
}
//---------------------------------------------------------------------

ID3D11DomainShader* CD3D11Shader::GetD3DDomainShader() const
{
	n_assert_dbg(Type == ShaderType_Domain);
	return static_cast<ID3D11DomainShader*>(pD3DShader);
}
//---------------------------------------------------------------------

ID3D11GeometryShader* CD3D11Shader::GetD3DGeometryShader() const
{
	n_assert_dbg(Type == ShaderType_Geometry);
	return static_cast<ID3D11GeometryShader*>(pD3DShader);
}
//---------------------------------------------------------------------

ID3D11PixelShader* CD3D11Shader::GetD3DPixelShader() const
{
	n_assert_dbg(Type == ShaderType_Pixel);
	return static_cast<ID3D11PixelShader*>(pD3DShader);
}
//---------------------------------------------------------------------

}