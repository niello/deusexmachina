#include "Shader.h"

#include <Render/Renderer.h>
#include <Data/Stream.h>
#include <Data/Buffer.h>
#include <d3dx9effect.h>

namespace Render
{

//!!!LOADER!
bool CShader::SetupFromStream(Data::CStream& Stream)
{
	DWORD FileSize = Stream.GetSize();
	Data::CBuffer Buffer(FileSize);
	n_assert(Stream.Read(Buffer.GetPtr(), FileSize) == FileSize);

//!!!Many of things below affect only compilation from source. For release, can be omitted!

	ID3DXBuffer* pErrorBuffer = NULL;
	DWORD D3DEffFlags = D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY;
#if N_D3D9_DEBUG
	D3DEffFlags |= (D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION);
#endif

//???TMP OLD?
	//CD3DXNebula2Include IncludeHandler(shaderPath.ExtractDirName());

	LPCSTR VSProfile = D3DXGetVertexShaderProfile(RenderSrv->GetD3DDevice());
	LPCSTR PSProfile = D3DXGetPixelShaderProfile(RenderSrv->GetD3DDevice());
	if (!VSProfile) VSProfile = "vs_2_0";
	if (!PSProfile) PSProfile = "ps_2_0";

	D3DXMACRO Defines[] =
	{
		{ "VS_PROFILE", VSProfile },
		{ "PS_PROFILE", PSProfile },
		{ NULL, NULL }
	};
// end OLD

	HRESULT hr = D3DXCreateEffect(RenderSrv->GetD3DDevice(), Buffer.GetPtr(), FileSize, Defines, NULL, D3DEffFlags,
		RenderSrv->GetD3DEffectPool(), &pEffect, &pErrorBuffer);

	if (FAILED(hr) || !pEffect)
	{
		n_error("CShader: failed to load fx file '%s' with:\n\n%s\n",
			"<path>",
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

//!!!inline!
void CShader::BeginPass(DWORD PassIdx)
{
	n_assert(SUCCEEDED(pEffect->BeginPass(PassIdx)));
	//n_dxtrace(pEffect->BeginPass(PassIdx), "nD3D9Shader:BeginPass() failed on effect");
}
//---------------------------------------------------------------------

}