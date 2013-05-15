// Loads shader effect in source form from .fx file
// There is only version for loading from file, cause FX source with includes is meaningful only as files.
// Use function declaration instead of header file where you want to call this loader.

#include <Render/RenderServer.h>
#include <Render/D3DXNebula2Include.h>
#include <IO/Streams/FileStream.h>
#include <Data/Buffer.h>

namespace Render
{

bool LoadShaderFromFX(const nString& FileName, const nString& ShaderRootDir, PShader OutShader)
{
	if (!OutShader.IsValid()) FAIL;

	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;

	DWORD FileSize = File.GetSize();
	Data::CBuffer Buffer(FileSize);
	n_assert(File.Read(Buffer.GetPtr(), FileSize) == FileSize);

	DWORD D3DEffFlags = D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY | D3DXFX_NOT_CLONEABLE;
#if DEM_D3D_DEBUG
	D3DEffFlags |= (D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION);
#endif

	CD3DXNebula2Include IncludeHandler(FileName.ExtractDirName(), ShaderRootDir);
	ID3DXBuffer* pErrorBuffer = NULL;
	ID3DXEffect* pEffect = NULL;

	HRESULT hr = D3DXCreateEffect(
		RenderSrv->GetD3DDevice(),
		Buffer.GetPtr(),
		FileSize,
		NULL,
		&IncludeHandler,
		D3DEffFlags,
		RenderSrv->GetD3DEffectPool(),
		&pEffect,
		&pErrorBuffer);

	if (FAILED(hr) || !pEffect)
	{
		n_message("FXLoader: failed to load fx file '%s' with:\n\n%s\n",
			FileName.CStr(),
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		FAIL;
	}
	else if (pErrorBuffer)
	{
		n_printf("FXLoader: fx file '%s' loaded with:\n\n%s\n", FileName.CStr(), pErrorBuffer->GetBufferPointer());
		if (pErrorBuffer) pErrorBuffer->Release();
	}

	return OutShader->Setup(pEffect);
}
//---------------------------------------------------------------------

}