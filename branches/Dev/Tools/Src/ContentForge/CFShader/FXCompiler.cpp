#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <Data/Buffer.h>
#include <Render/D3DXDEMInclude.h>
#include <ConsoleApp.h>
#include <d3dx9.h>

#undef CreateDirectory

extern CString RootPath;

int CompileShader(const CString& InFilePath, const CString& OutFilePath, bool Debug, int OptimizationLevel)
{
	Data::CBuffer In;
	if (!IOSrv->LoadFileToBuffer(InFilePath, In)) return ERR_IO_READ;

	//???D3DXSHADER_IEEE_STRICTNESS
	DWORD Flags = D3DXFX_NOT_CLONEABLE;
	if (Debug)
	{
		Flags |= (D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION);
		n_msg(VL_DEBUG, "Debug compilation on\n");
	}
	else
	{
		switch (OptimizationLevel)
		{
			case 0:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL0; break;
			case 1:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL1; break;
			case 2:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL2; break;
			default:	Flags |= D3DXSHADER_OPTIMIZATION_LEVEL3; break;
		}
	}

	CD3DXDEMInclude IncHandler(InFilePath.ExtractDirName(), RootPath);

	ID3DXEffectCompiler* pCompiler = NULL;
	ID3DXBuffer* pErrorBuffer = NULL;
	HRESULT hr =
		D3DXCreateEffectCompiler((LPCSTR)In.GetPtr(), In.GetSize(), NULL, &IncHandler, Flags, &pCompiler, &pErrorBuffer);

	if (FAILED(hr) || !pCompiler)
	{
		n_msg(VL_ERROR, "Failed to load FX file '%s' with:\n\n%s\n",
			InFilePath.CStr(),
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		return ERR_MAIN_FAILED;
	}
	else if (pErrorBuffer)
	{
		n_msg(VL_WARNING, "FX file '%s' loaded with:\n\n%s\n", InFilePath.CStr(), pErrorBuffer->GetBufferPointer());
		pErrorBuffer->Release();
	}

	ID3DXBuffer* pEffect = NULL;
	hr = pCompiler->CompileEffect(Flags, &pEffect, &pErrorBuffer);

	if (FAILED(hr) || !pEffect)
	{
		n_msg(VL_ERROR, "Failed to compile FXO file '%s' with:\n\n%s\n",
			OutFilePath.CStr(),
			pErrorBuffer ? pErrorBuffer->GetBufferPointer() : "No D3DX error message.");
		if (pErrorBuffer) pErrorBuffer->Release();
		return ERR_MAIN_FAILED;
	}
	else if (pErrorBuffer)
	{
		n_msg(VL_WARNING, "FXO file '%s' compiled with:\n\n%s\n", OutFilePath.CStr(), pErrorBuffer->GetBufferPointer());
		pErrorBuffer->Release();
	}

	IOSrv->CreateDirectory(OutFilePath.ExtractDirName());

	IO::CFileStream File;
	if (!File.Open(OutFilePath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	if (File.Write(pEffect->GetBufferPointer(), pEffect->GetBufferSize()) != pEffect->GetBufferSize()) return ERR_IO_WRITE;

	return SUCCESS;
}
//---------------------------------------------------------------------
