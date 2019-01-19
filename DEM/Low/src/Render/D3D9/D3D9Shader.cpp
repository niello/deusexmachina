#include "D3D9Shader.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9Shader, 'SHD9', Render::CShader);

CD3D9Shader::~CD3D9Shader()
{
	InternalDestroy();
}
//---------------------------------------------------------------------

bool CD3D9Shader::Create(IUnknown* pShader)
{
	if (!pShader) FAIL;

	IDirect3DVertexShader9* pVS = nullptr;
	IDirect3DPixelShader9* pPS = nullptr;
	if (SUCCEEDED(pShader->QueryInterface(&pVS)))
	{
		if (Create(pVS))
		{
			pShader->Release(); // QueryInterface adds ref to PS, so we clear initial ref
			OK;
		}
	}
	else if (SUCCEEDED(pShader->QueryInterface(&pPS)))
	{
		if (Create(pPS))
		{
			pShader->Release(); // QueryInterface adds ref to PS, so we clear initial ref
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D9Shader::Create(IDirect3DVertexShader9* pShader)
{
	if (!pShader) FAIL;
	Type = ShaderType_Vertex;
	pD3DVertexShader = pShader;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9Shader::Create(IDirect3DPixelShader9* pShader)
{
	if (!pShader) FAIL;
	Type = ShaderType_Pixel;
	pD3DPixelShader = pShader;
	OK;
}
//---------------------------------------------------------------------

void CD3D9Shader::InternalDestroy()
{
	if (Type == ShaderType_Vertex)
	{
		SAFE_RELEASE(pD3DVertexShader);
	}
	else if (Type == ShaderType_Pixel)
	{
		SAFE_RELEASE(pD3DPixelShader);
	}
	else n_assert(!pD3DShader);

	Metadata.Clear();
}
//---------------------------------------------------------------------

}