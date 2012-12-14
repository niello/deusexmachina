// Loads shader effect in source form from .fx file
// There is only version for loading from file, cause FX source with includes is meaningful only as files.
// Use function declaration instead of header file where you want to call this loader.

#include <Render/RenderServer.h>
#include <Data/Streams/FileStream.h>
#include <Data/Buffer.h>
#include <gfx2/D3DXNebula2Include.h>

namespace Render
{

bool LoadShaderFromFX(const nString& FileName, PShader OutShader)
{
	if (!OutShader.isvalid()) FAIL;

	Data::CFileStream File;
	if (!File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL)) FAIL;

	DWORD FileSize = File.GetSize();
	Data::CBuffer Buffer(FileSize);
	n_assert(File.Read(Buffer.GetPtr(), FileSize) == FileSize);

	DWORD D3DEffFlags = D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY | D3DXFX_NOT_CLONEABLE;
#if DEM_D3D_DEBUG
	D3DEffFlags |= (D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION);
#endif

//!!!OLD+
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

	CD3DXNebula2Include IncludeHandler(FileName.ExtractDirName());
//!!!OLD-

	ID3DXBuffer* pErrorBuffer = NULL;
	ID3DXEffect* pEffect = NULL;

	HRESULT hr = D3DXCreateEffect(
		RenderSrv->GetD3DDevice(),
		Buffer.GetPtr(),
		FileSize,
		Defines,
		&IncludeHandler,
		D3DEffFlags,
		RenderSrv->GetD3DEffectPool(),
		&pEffect,
		&pErrorBuffer);

	if (FAILED(hr) || !pEffect)
	{
		n_printf("FXLoader: failed to load fx file '%s' with:\n\n%s\n",
			FileName.Get(),
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		FAIL;
	}

	return OutShader->Setup(pEffect);
}
//---------------------------------------------------------------------

}