// Loads shader effect in object form from .fxo file
// Use function declaration instead of header file where you want to call this loader.

#include <Render/RenderServer.h>
#include <Data/Streams/FileStream.h>
#include <Data/Buffer.h>

namespace Render
{

bool LoadShaderFromFXO(Data::CStream& In, PShader OutShader)
{
	if (!OutShader.isvalid()) FAIL;

	DWORD DataSize = In.GetSize();
	Data::CBuffer Buffer(DataSize);
	n_assert(In.Read(Buffer.GetPtr(), DataSize) == DataSize);

	ID3DXBuffer* pErrorBuffer = NULL;
	ID3DXEffect* pEffect = NULL;

	HRESULT hr = D3DXCreateEffect(
		RenderSrv->GetD3DDevice(),
		Buffer.GetPtr(),
		DataSize,
		NULL,
		NULL,
		D3DXFX_NOT_CLONEABLE,			// 0 or D3DXFX_NOT_CLONEABLE - 50% less memory but can't clone
		RenderSrv->GetD3DEffectPool(),
		&pEffect,
		&pErrorBuffer);

	if (FAILED(hr) || !pEffect)
	{
		n_printf("FXLoader: failed to load FXO shader with:\n\n%s\n",
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		FAIL;
	}

	return OutShader->Setup(pEffect);
}
//---------------------------------------------------------------------

bool LoadShaderFromFXO(const nString& FileName, PShader OutShader)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadShaderFromFXO(File, OutShader);
}
//---------------------------------------------------------------------

}