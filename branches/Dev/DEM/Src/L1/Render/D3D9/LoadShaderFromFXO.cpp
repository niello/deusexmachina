//// Loads shader effect in object form from .fxo file
//// Use function declaration instead of header file where you want to call this loader.
//
//#include <Render/RenderServer.h>
//#include <IO/Streams/FileStream.h>
//#include <Data/Buffer.h>
//
//#ifdef DEM_USE_D3DX9
//#define WIN32_LEAN_AND_MEAN
//#define D3D_DISABLE_9EX
//#include <d3dx9.h>
//#endif
//
//namespace Render
//{
//
//bool LoadShaderFromFXO(IO::CStream& In, PShader OutShader)
//{
//	if (!OutShader.IsValid()) FAIL;
//
//	DWORD DataSize = In.GetSize();
//	Data::CBuffer Buffer(DataSize);
//	n_assert(In.Read(Buffer.GetPtr(), DataSize) == DataSize);
//
//	ID3DXBuffer* pErrorBuffer = NULL;
//	ID3DXEffect* pEffect = NULL;
//
//	HRESULT hr = D3DXCreateEffect(
//		RenderSrv->GetD3DDevice(),
//		Buffer.GetPtr(),
//		DataSize,
//		NULL,
//		NULL,
//		D3DXFX_NOT_CLONEABLE,			// 0 or D3DXFX_NOT_CLONEABLE - 50% less memory but can't clone
//		RenderSrv->GetD3DEffectPool(),
//		&pEffect,
//		&pErrorBuffer);
//
//	if (FAILED(hr) || !pEffect)
//	{
//		Sys::Log("FXLoader: failed to load FXO shader with:\n\n%s\n",
//			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
//		if (pErrorBuffer) pErrorBuffer->Release();
//		FAIL;
//	}
//
//	return OutShader->Setup(pEffect);
//}
////---------------------------------------------------------------------
//
//bool LoadShaderFromFXO(const CString& FileName, PShader OutShader)
//{
//	IO::CFileStream File;
//	return File.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL) &&
//		LoadShaderFromFXO(File, OutShader);
//}
////---------------------------------------------------------------------
//
//}