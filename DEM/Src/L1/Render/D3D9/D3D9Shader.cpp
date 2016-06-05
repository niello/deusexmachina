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
	Metadata.Clear();
}
//---------------------------------------------------------------------

}