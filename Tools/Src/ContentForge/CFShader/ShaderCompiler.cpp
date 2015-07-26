#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <Data/Buffer.h>
#include <ConsoleApp.h>
#include <DEMD3DInclude.h>
#include <D3DCompiler.h>

#undef CreateDirectory

extern CString RootPath;

int CompileShader(const CString& InFilePath, const CString& OutFilePath, bool Debug, int OptimizationLevel)
{
	Data::CBuffer In;
	if (!IOSrv->LoadFileToBuffer(InFilePath, In)) return ERR_IO_READ;

	// D3DCOMPILE_DEBUG - debug info
	// D3DCOMPILE_SKIP_OPTIMIZATION - only for active debug
	// D3DCOMPILE_SKIP_VALIDATION - faster, if successfully compiled
	// D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR (more efficient, vec*mtx dots)
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL
	// D3DCOMPILE_ENABLE_STRICTNESS - kill deprecated syntax

	// D3D_COMPILE_STANDARD_FILE_INCLUDE

	//D3DCompile
	//D3DCompile2
	//D3DPreprocess - apply preprocessor to text
	//D3DGetBlobPart, D3D_BLOB_INPUT_SIGNATURE_BLOB
	//D3DStripShader - remove blobs
	//D3DCompressShaders
	//D3DDecompressShaders

	DWORD Flags = 0;
	if (Debug)
	{
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
		n_msg(VL_DEBUG, "Debug compilation on\n");
	}
	else
	{
		//switch (OptimizationLevel)
		//{
		//	case 0:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL0; break;
		//	case 1:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL1; break;
		//	case 2:		Flags |= D3DXSHADER_OPTIMIZATION_LEVEL2; break;
		//	default:	Flags |= D3DXSHADER_OPTIMIZATION_LEVEL3; break;
		//}
	}

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(InFilePath), RootPath);

	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), InFilePath, Defines, &IncHandler, Entry, Target, Fl1, Fl2, &pOutCode, &pOutErrors);
/*
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
*/

	IO::CFileStream File;
	if (!File.Open(OutFilePath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	if (File.Write(pEffect->GetBufferPointer(), pEffect->GetBufferSize()) != pEffect->GetBufferSize()) return ERR_IO_WRITE;

	return SUCCESS;
}
//---------------------------------------------------------------------
