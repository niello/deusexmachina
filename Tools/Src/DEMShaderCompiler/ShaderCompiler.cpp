#include "ShaderCompiler.h"

#include <Data/FourCC.h>
#include <Data/Buffer.h>
#include <Data/Dictionary.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/PathUtils.h>
#include <Util/UtilFwd.h>
#include <DEMD3DInclude.h>
#include <ShaderDB.h>
#include <ShaderReflection.h>

//!!!n_msg, logging! can out to stdout / stderr

#undef CreateDirectory
#undef DeleteFile

struct CTargetParams
{
	const char*		pD3DTarget;
	const char*		pExtension;
	Data::CFourCC	FileSignature;
};

class CAutoFree
{
private:

	void* pDataToFree;

public:

	CAutoFree(): pDataToFree(NULL) {}
	~CAutoFree() { if (pDataToFree) n_free(pDataToFree); }

	void Set(void* pData) { pDataToFree = pData; }
};

DEM_DLL_EXPORT bool DEM_DLLCALL OpenShaderDatabase(const char* pDBFilePath)
{
	return OpenDB(pDBFilePath);
}
//---------------------------------------------------------------------

inline U32 GetTargetByFileSignature(Data::CFourCC FileSig)
{
	// File signature always stores target in bytes 1 and 0
	//!!!add endian-correctness!
	char TargetHigh = FileSig.GetChar(1) - '0';
	char TargetLow = FileSig.GetChar(0) - '0';
	return (TargetHigh << 8) | TargetLow;
}
//---------------------------------------------------------------------

bool GetTargetParams(EShaderType ShaderType, U32 Target, CTargetParams& Out)
{
	const char* pTarget = NULL;
	const char* pExt = NULL;
	Data::CFourCC FileSig;
	switch (ShaderType)
	{
		case ShaderType_Vertex:
		{
			pExt = "vsh";
			switch (Target)
			{
				case 0x0500: pTarget = "vs_5_0"; FileSig.Code = 'VS50'; break;
				case 0x0401: pTarget = "vs_4_1"; FileSig.Code = 'VS41'; break;
				case 0x0400: pTarget = "vs_4_0"; FileSig.Code = 'VS40'; break;
				case 0x0300: pTarget = "vs_3_0"; FileSig.Code = 'VS30'; break;
				default: FAIL;
			}
			break;
		}
		case ShaderType_Pixel:
		{
			pExt = "psh";
			switch (Target)
			{
				case 0x0500: pTarget = "ps_5_0"; FileSig.Code = 'PS50'; break;
				case 0x0401: pTarget = "ps_4_1"; FileSig.Code = 'PS41'; break;
				case 0x0400: pTarget = "ps_4_0"; FileSig.Code = 'PS40'; break;
				case 0x0300: pTarget = "ps_3_0"; FileSig.Code = 'PS30'; break;
				default: FAIL;
			}
			break;
		}
		case ShaderType_Geometry:
		{
			pExt = "gsh";
			switch (Target)
			{
				case 0x0500: pTarget = "gs_5_0"; FileSig.Code = 'GS50'; break;
				case 0x0401: pTarget = "gs_4_1"; FileSig.Code = 'GS41'; break;
				case 0x0400: pTarget = "gs_4_0"; FileSig.Code = 'GS40'; break;
				default: FAIL;
			}
			break;
		}
		case ShaderType_Hull:
		{
			pExt = "hsh";
			switch (Target)
			{
				case 0x0500: pTarget = "hs_5_0"; FileSig.Code = 'HS50'; break;
				default: FAIL;
			}
			break;
		}
		case ShaderType_Domain:
		{
			pExt = "dsh";
			switch (Target)
			{
				case 0x0500: pTarget = "ds_5_0"; FileSig.Code = 'DS50'; break;
				default: FAIL;
			}
			break;
		}
		default: FAIL;
	};

	Out.pD3DTarget = pTarget;
	Out.pExtension = pExt;
	Out.FileSignature = FileSig;

	OK;
}
//---------------------------------------------------------------------

// String passed will be tokenized in place, so it must be read-write accessible.
// Memory management is a caller's responsibility.
bool ParseDefineString(char* pDefineString, CArray<CMacroDBRec>& Out)
{
	CMacroDBRec CurrMacro = { 0 };

	char* pCurrPos = pDefineString;
	const char* pBothDlms = "=;";
	const char* pSemicolonOnly = ";";
	const char* pCurrDlms = pBothDlms;
	while (true)
	{
		char* pDlm = strpbrk(pCurrPos, pCurrDlms);
		if (pDlm)
		{
			char Dlm = *pDlm;
			if (Dlm == '=')
			{
				CurrMacro.Name = pCurrPos;
				CurrMacro.Value = pDlm + 1;
				Out.Add(CurrMacro);
				pCurrDlms = pSemicolonOnly;
			}
			else // ';'
			{
				CurrMacro.Value = NULL;
				if (!CurrMacro.Name)
				{
					CurrMacro.Name = pCurrPos;
					Out.Add(CurrMacro);
				}
				CurrMacro.Name = NULL;
				pCurrDlms = pBothDlms;
			}
			*pDlm = 0;
			pCurrPos = pDlm + 1;
		}
		else
		{
			CurrMacro.Value = NULL;
			if (!CurrMacro.Name)
			{
				CurrMacro.Name = pCurrPos;
				Out.Add(CurrMacro);
			}
			CurrMacro.Name = NULL;
			break;
		}
	}

	// CurrMacro is both NULLs in all control pathes here, but we don't add it.
	// CompileShader() method takes care of it for D3D.

	OK;
}
//---------------------------------------------------------------------

// pDefines - "NAME[=VALUE];NAME[=VALUE];...NAME[=VALUE]"
DEM_DLL_EXPORT bool DEM_DLLCALL CompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
											  const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID)
{
	if (!pSrcPath || ShaderType >= ShaderType_COUNT || !pEntryPoint) FAIL;

	if (!Target) Target = 0x0500;

	IO::CFileSystemWin32 FS;

	Data::CBuffer In;
	void* hFile = FS.OpenFile(pSrcPath, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!hFile) FAIL;
	U64 CurrWriteTime = FS.GetFileWriteTime(hFile);
	UPTR FileSize = (UPTR)FS.GetFileSize(hFile);
	In.Reserve(FileSize);
	UPTR ReadSize = FS.Read(hFile, In.GetPtr(), FileSize);
	FS.CloseFile(hFile);
		
	if (ReadSize != FileSize) FAIL;

	UPTR SrcCRC = Util::CalcCRC((U8*)In.GetPtr(), In.GetSize());

	// Setup compiler flags

	// D3DCOMPILE_IEEE_STRICTNESS
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL

	//DWORD Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; // (more efficient, vec*mtx dots) //???does touch CPU-side const binding code?
	DWORD Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	if (Target >= 0x0400)
	{
		Flags |= D3DCOMPILE_ENABLE_STRICTNESS; // Denies deprecated syntax
	}
	else
	{
		//Flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
	}

	if (Debug)
	{
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
	}
	else Flags |= (D3DCOMPILE_OPTIMIZATION_LEVEL3); // | D3DCOMPILE_SKIP_VALIDATION);

	// Get info about previous compilation, skip if no changes

	CShaderDBRec Rec;
	Rec.SrcFile.Path = pSrcPath;
	Rec.ShaderType = ShaderType;
	Rec.Target = Target;
	Rec.EntryPoint = pEntryPoint;

	char* pDefineStringBuffer;
	CAutoFree DefineStringBuffer;
	if (pDefines && *pDefines)
	{
		UPTR DefinesLen = strlen(pDefines) + 1;
		pDefineStringBuffer = (char*)n_malloc(DefinesLen);
		strcpy_s(pDefineStringBuffer, DefinesLen, pDefines);
		DefineStringBuffer.Set(pDefineStringBuffer);
		if (!ParseDefineString(pDefineStringBuffer, Rec.Defines)) FAIL;
	}

	bool RecFound = FindShaderRec(Rec);
	if (RecFound &&
		Rec.CompilerFlags == Flags &&
		Rec.SrcFile.Size == In.GetSize() &&
		Rec.SrcFile.CRC == SrcCRC &&
		CurrWriteTime &&
		Rec.SrcModifyTimestamp == CurrWriteTime)
	{
		OK;
	}

	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = In.GetSize();
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Determine D3D target, output file extension and file signature

	CTargetParams TargetParams;
	if (!GetTargetParams(ShaderType, Target, TargetParams)) FAIL;

	// Setup D3D shader macros

	CArray<D3D_SHADER_MACRO> D3DMacros(Rec.Defines.GetCount() + 1, 0);
	D3D_SHADER_MACRO* pD3DMacros;
	if (Rec.Defines.GetCount())
	{
		for (UPTR i = 0; i < Rec.Defines.GetCount(); ++i)
		{
			const CMacroDBRec& Macro = Rec.Defines[i];
			D3D_SHADER_MACRO* pD3DMacro = D3DMacros.Add();
			pD3DMacro->Name = Macro.Name;
			pD3DMacro->Definition = Macro.Value;
		}

		// Terminating macro
		D3D_SHADER_MACRO D3DMacro = { NULL, NULL };
		D3DMacros.Add(D3DMacro);

		pD3DMacros = &D3DMacros[0];
	}
	else pD3DMacros = NULL;

	// Compile shader

	//CString ShortSrcPath(pSrcPath);
	//ShortSrcPath.Replace(IOSrv->ResolveAssigns("SrcShaders:") + "/", "SrcShaders:");

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(pSrcPath), CString::Empty); //RootPath);

	ID3DBlob* pCode = NULL;
	ID3DBlob* pErrors = NULL;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), pSrcPath,
							pD3DMacros, &IncHandler, pEntryPoint, TargetParams.pD3DTarget,
							Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		//n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n",
			//ShortSrcPath.CStr(),
			//pErrors ? pErrors->GetBufferPointer() : "No D3D error message.");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		FAIL;
	}
	else if (pErrors)
	{
		//n_msg(VL_WARNING, "'%s' compiled with warnings:\n\n%s\n", ShortSrcPath.CStr(), pErrors->GetBufferPointer());
		pErrors->Release();
	}

	// For vertex and geometry shaders, store input signature in a separate binary file.
	// It saves RAM since input signatures must reside in it at the runtime.

	if ((ShaderType == ShaderType_Vertex || ShaderType == ShaderType_Geometry) && Target >= 0x0400)
	{
		ID3DBlob* pInputSig;
		if (SUCCEEDED(D3DGetBlobPart(pCode->GetBufferPointer(),
									 pCode->GetBufferSize(),
									 D3D_BLOB_INPUT_SIGNATURE_BLOB,
									 0,
									 &pInputSig)))
		{
			Rec.InputSigFile.Size = pInputSig->GetBufferSize();
			Rec.InputSigFile.CRC = Util::CalcCRC((U8*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());

			U32 OldInputSigID = Rec.InputSigFile.ID;
			if (!FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer(), false))
			{
				if (!RegisterObjFile(Rec.InputSigFile, "sig")) // Fills empty ID and path inside
				{
					pCode->Release();
					pInputSig->Release();
					FAIL;
				}

				//n_msg(VL_DETAILS, "  InputSig: %s -> %s\n", ShortSrcPath.CStr(), Rec.InputSigFile.Path.CStr());

				FS.CreateDirectory(PathUtils::ExtractDirName(Rec.InputSigFile.Path));

				void* hFile = FS.OpenFile(Rec.InputSigFile.Path.CStr(), IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
				if (!hFile)
				{
					pCode->Release();
					pInputSig->Release();
					FAIL;
				}
				FS.Write(hFile, pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());
				FS.CloseFile(hFile);
			}

			if (OldInputSigID > 0 && OldInputSigID != Rec.InputSigFile.ID)
			{
				CString OldObjPath;
				if (ReleaseObjFile(OldInputSigID, OldObjPath))
					FS.DeleteFile(OldObjPath);
			}

			pInputSig->Release();
		}
		else Rec.InputSigFile.ID = 0;
	}
	else Rec.InputSigFile.ID = 0;

	// Strip unnecessary info for release builds, making object files smaller

	ID3DBlob* pFinalCode;
	if (Debug || Target < 0x0400)
	{
		pFinalCode = pCode;
		pFinalCode->AddRef();
	}
	else
	{
		hr = D3DStripShader(pCode->GetBufferPointer(),
							pCode->GetBufferSize(),
							D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
							&pFinalCode);
		if (FAILED(hr))
		{
			pCode->Release();
			FAIL;
		}
	}

	Rec.ObjFile.Size = pFinalCode->GetBufferSize();
	Rec.ObjFile.CRC = Util::CalcCRC((U8*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	if (!FindObjFile(Rec.ObjFile, pFinalCode->GetBufferPointer(), true))
	{
		if (!RegisterObjFile(Rec.ObjFile, TargetParams.pExtension)) // Fills empty ID and path inside
		{
			pCode->Release();
			pFinalCode->Release();
			FAIL;
		}

		//n_msg(VL_DETAILS, "  Shader:   %s -> %s\n", ShortSrcPath.CStr(), Rec.ObjFile.Path.CStr());

		FS.CreateDirectory(PathUtils::ExtractDirName(Rec.ObjFile.Path));

		IO::CFileStream File(Rec.ObjFile.Path, &FS);
		if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			pFinalCode->Release();
			FAIL;
		}

		IO::CBinaryWriter W(File);

		W.Write(TargetParams.FileSignature);

		// Offset of a shader binary, will fill later
		U64 OffsetOffset = File.GetPosition();
		W.Write<U32>(0);

		W.Write<U32>(Rec.ObjFile.ID);

		if (Target >= 0x0400)
		{
			W.Write<U32>(Rec.InputSigFile.ID);
		}

		// Save metadata

		bool MetaReflected;
		bool MetaSaved;
		if (Target >= 0x0400)
		{
			CD3D11ShaderMeta Meta;
			MetaReflected = D3D11CollectShaderMetadata(pCode->GetBufferPointer(), pCode->GetBufferSize(), Meta);
			MetaSaved = MetaReflected && D3D11SaveShaderMetadata(W, Meta);
		}
		else
		{
			CSM30ShaderMeta Meta;
			MetaReflected = D3D9CollectShaderMetadata(pCode->GetBufferPointer(), pCode->GetBufferSize(), (const char*)In.GetPtr(), In.GetSize(), Meta);
			MetaSaved = MetaReflected && D3D9SaveShaderMetadata(W, Meta);
		}

		pCode->Release();
		
		if (!MetaReflected || !MetaSaved)
		{
			//n_msg(VL_ERROR, "	Shader %s failed: '%s'\n", MetaReflected ? "metadata saving" : "reflection", Rec.ObjFile.Path.CStr());
			pFinalCode->Release();
			FAIL;
		}

		// Save shader binary
		U64 BinaryOffset = File.GetPosition();
		File.Write(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

		// Write binary data offset for fast skipping of metadata when reading
		File.Seek(OffsetOffset, IO::Seek_Begin);
		W.Write<U32>((U32)BinaryOffset);

		File.Close();
	}
	else
	{
		pCode->Release();
	}

	if (OldObjFileID > 0 && OldObjFileID != Rec.ObjFile.ID)
	{
		CString OldObjPath;
		if (ReleaseObjFile(OldObjFileID, OldObjPath))
			FS.DeleteFile(OldObjPath);
	}

	if (!WriteShaderRec(Rec)) FAIL;

	ObjectFileID = Rec.ObjFile.ID;
	InputSignatureFileID = Rec.InputSigFile.ID;

	OK;
}
//---------------------------------------------------------------------

// Since D3D9 and D3D11 metadata are different, we implement not beautiful but convenient function,
// that returns both D3D9 and D3D11 metadata pointers. Which one is not NULL, it must be used as a return value.
// Returns whether metadata is found in cache, which means it was already processed.
DEM_DLL_EXPORT bool DEM_DLLCALL LoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta& OutD3D9Meta, CD3D11ShaderMeta& OutD3D11Meta)
{
	CFileData ObjFile;
	if (!FindObjFileByID(ID, ObjFile)) FAIL;

	IO::CFileStream File(ObjFile.Path.CStr());
	if (!File.Open(IO::SAM_READ)) FAIL;
	IO::CBinaryReader R(File);

	Data::CFourCC FileSig;
	R.Read(FileSig.Code);
	OutTarget = GetTargetByFileSignature(FileSig);

	R.Read<U32>();	// Binary data offset - skip
	R.Read<U32>();	// Shader obj file ID - skip

	if (OutTarget >= 0x0400)
	{
		R.Read<U32>();	// Input signature obj file ID - skip
		return D3D11LoadShaderMetadata(R, OutD3D11Meta);
	}
	else return D3D9LoadShaderMetadata(R, OutD3D9Meta);
}
//---------------------------------------------------------------------
