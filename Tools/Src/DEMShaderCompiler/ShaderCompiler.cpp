#include "ShaderCompiler.h"

#include <Data/Buffer.h>
#include <Data/StringUtils.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/PathUtils.h>
#include <Util/UtilFwd.h>
#include <DEMD3DInclude.h>
#include <ShaderDB.h>
#include <ShaderReflection.h>
#include <ValueTable.h>

#undef CreateDirectory
#undef DeleteFile

CString OutputDir;
CString Messages;	// Last operation messages. Must be cleared at the start of every public API function.

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

DEM_DLL_EXPORT bool DEM_DLLCALL InitCompiler(const char* pDBFileName, const char* pOutputDirectory)
{
	Messages.Clear();

	if (pOutputDirectory)
	{
		OutputDir.Set(pOutputDirectory);
		PathUtils::EnsurePathHasEndingDirSeparator(OutputDir);
	}
	else
	{
		OutputDir = PathUtils::ExtractDirName(pDBFileName);
		OutputDir += "../../Export/Shaders/Bin/";
		OutputDir = PathUtils::CollapseDots(OutputDir.CStr());
	}

	return OpenDB(pDBFileName);
}
//---------------------------------------------------------------------

DEM_DLL_EXPORT const char* DEM_DLLCALL GetLastOperationMessages()
{
	return Messages.CStr();
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
DEM_DLL_EXPORT int DEM_DLLCALL CompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
											 const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID)
{
	Messages.Clear();

	if (!pSrcPath || ShaderType >= ShaderType_COUNT || !pEntryPoint) return DEM_SHADER_COMPILER_INVALID_ARGS;

	if (!Target) Target = 0x0500;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);

	Data::CBuffer In;
	void* hFile = FS->OpenFile(pSrcPath, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!hFile) return DEM_SHADER_COMPILER_IO_READ_ERROR;
	U64 CurrWriteTime = FS->GetFileWriteTime(hFile);
	UPTR FileSize = (UPTR)FS->GetFileSize(hFile);
	In.Reserve(FileSize);
	UPTR ReadSize = FS->Read(hFile, In.GetPtr(), FileSize);
	FS->CloseFile(hFile);
		
	if (ReadSize != FileSize) return DEM_SHADER_COMPILER_IO_READ_ERROR;

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
		if (!ParseDefineString(pDefineStringBuffer, Rec.Defines)) return DEM_SHADER_COMPILER_INVALID_ARGS;
	}

	bool RecFound = FindShaderRec(Rec);
	if (RecFound &&
		Rec.CompilerFlags == Flags &&
		Rec.SrcFile.Size == In.GetSize() &&
		Rec.SrcFile.CRC == SrcCRC &&
		CurrWriteTime &&
		Rec.SrcModifyTimestamp == CurrWriteTime)
	{
		ObjectFileID = Rec.ObjFile.ID;
		InputSignatureFileID = Rec.InputSigFile.ID;
		return DEM_SHADER_COMPILER_SUCCESS;
	}

	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = In.GetSize();
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Determine D3D target, output file extension and file signature

	CTargetParams TargetParams;
	if (!GetTargetParams(ShaderType, Target, TargetParams)) return DEM_SHADER_COMPILER_INVALID_ARGS;

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

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(pSrcPath), CString::Empty); //RootPath);

	ID3DBlob* pCode = NULL;
	ID3DBlob* pErrors = NULL;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), pSrcPath,
							pD3DMacros, &IncHandler, pEntryPoint, TargetParams.pD3DTarget,
							Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		Messages.Set(pErrors ? (const char*)pErrors->GetBufferPointer() : "<No D3D error message>");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		return DEM_SHADER_COMPILER_COMPILE_ERROR;
	}
	else if (pErrors)
	{
		Messages.Set("Compiled with warnings:\n\n");
		Messages += (const char*)pErrors->GetBufferPointer();
		Messages += '\n';
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
			Rec.InputSigFile.BytecodeSize = pInputSig->GetBufferSize();
			Rec.InputSigFile.CRC = Util::CalcCRC((U8*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());

			U32 OldInputSigID = Rec.InputSigFile.ID;
			if (!FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer(), false))
			{
				Rec.InputSigFile.ID = CreateObjFileRecord();
				if (!Rec.InputSigFile.ID)
				{
					pCode->Release();
					pInputSig->Release();
					return DEM_SHADER_COMPILER_DB_ERROR;
				}

				Rec.InputSigFile.Path = PathUtils::CollapseDots(OutputDir + StringUtils::FromInt(Rec.InputSigFile.ID) + ".sig");

				FS->CreateDirectory(PathUtils::ExtractDirName(Rec.InputSigFile.Path));

				void* hFile = FS->OpenFile(Rec.InputSigFile.Path.CStr(), IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
				if (!hFile)
				{
					pCode->Release();
					pInputSig->Release();
					return DEM_SHADER_COMPILER_IO_WRITE_ERROR;
				}
				FS->Write(hFile, pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());
				FS->CloseFile(hFile);

				if (!UpdateObjFileRecord(Rec.InputSigFile))
				{
					pCode->Release();
					pInputSig->Release();
					return DEM_SHADER_COMPILER_DB_ERROR;
				}
			}

			if (OldInputSigID > 0 && OldInputSigID != Rec.InputSigFile.ID)
			{
				CString OldObjPath;
				if (ReleaseObjFile(OldInputSigID, OldObjPath))
					FS->DeleteFile(OldObjPath);
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
			Messages += "\nD3DStripShader() failed\n";
			pCode->Release();
			return DEM_SHADER_COMPILER_ERROR;
		}
	}

	Rec.ObjFile.BytecodeSize = pFinalCode->GetBufferSize();
	Rec.ObjFile.CRC = Util::CalcCRC((U8*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	if (!FindObjFile(Rec.ObjFile, pFinalCode->GetBufferPointer(), true))
	{
		Rec.ObjFile.ID = CreateObjFileRecord();
		if (!Rec.ObjFile.ID)
		{
			pCode->Release();
			pFinalCode->Release();
			return DEM_SHADER_COMPILER_DB_ERROR;
		}

		Rec.ObjFile.Path = PathUtils::CollapseDots(OutputDir + StringUtils::FromInt(Rec.ObjFile.ID) + "." + TargetParams.pExtension);

		FS->CreateDirectory(PathUtils::ExtractDirName(Rec.ObjFile.Path));

		IO::CFileStream File(Rec.ObjFile.Path, FS);
		if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			pCode->Release();
			pFinalCode->Release();
			return DEM_SHADER_COMPILER_IO_WRITE_ERROR;
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
			pFinalCode->Release();
			return MetaReflected ? DEM_SHADER_COMPILER_IO_WRITE_ERROR : DEM_SHADER_COMPILER_REFLECTION_ERROR;
		}

		// Save shader binary
		U64 BinaryOffset = File.GetPosition();
		File.Write(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

		pFinalCode->Release();

		// Get total file size
		Rec.ObjFile.Size = File.GetPosition();

		// Write binary data offset for fast skipping of metadata when reading
		File.Seek(OffsetOffset, IO::Seek_Begin);
		W.Write<U32>((U32)BinaryOffset);

		File.Close();

		if (!UpdateObjFileRecord(Rec.ObjFile)) return DEM_SHADER_COMPILER_DB_ERROR;
	}
	else
	{
		pCode->Release();
		pFinalCode->Release();
	}

	if (OldObjFileID > 0 && OldObjFileID != Rec.ObjFile.ID)
	{
		CString OldObjPath;
		if (ReleaseObjFile(OldObjFileID, OldObjPath))
			FS->DeleteFile(OldObjPath);
	}

	if (!WriteShaderRec(Rec)) return DEM_SHADER_COMPILER_DB_ERROR;

	ObjectFileID = Rec.ObjFile.ID;
	InputSignatureFileID = Rec.InputSigFile.ID;

	return DEM_SHADER_COMPILER_SUCCESS;
}
//---------------------------------------------------------------------

// Since D3D9 and D3D11 metadata are different, we implement not beautiful but convenient function,
// that returns both D3D9 and D3D11 metadata pointers. Which one is not NULL, it must be used as a return value.
// Returns whether metadata is found in cache, which means it was already processed.
// Use FreeShaderMetadata() on pointers returned
DEM_DLL_EXPORT bool DEM_DLLCALL LoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta)
{
	Messages.Clear();

	CObjFileData ObjFile;
	if (!FindObjFileByID(ID, ObjFile)) FAIL;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);
	IO::CFileStream File(ObjFile.Path.CStr(), FS);
	if (!File.Open(IO::SAM_READ)) FAIL;
	IO::CBinaryReader R(File);

	Data::CFourCC FileSig;
	R.Read(FileSig.Code);
	OutTarget = GetTargetByFileSignature(FileSig);

	R.Read<U32>();	// Binary data offset - skip
	R.Read<U32>();	// Shader obj file ID - skip

	pOutD3D9Meta = NULL;
	pOutD3D11Meta = NULL;

	if (OutTarget >= 0x0400)
	{
		R.Read<U32>();	// Input signature obj file ID - skip
		pOutD3D11Meta = n_new(CD3D11ShaderMeta);
		if (D3D11LoadShaderMetadata(R, *pOutD3D11Meta)) OK;
		else
		{
			n_delete(pOutD3D11Meta);
			pOutD3D11Meta = NULL;
			FAIL;
		}
	}
	else
	{
		pOutD3D9Meta = n_new(CSM30ShaderMeta);
		if (D3D9LoadShaderMetadata(R, *pOutD3D9Meta)) OK;
		else
		{
			n_delete(pOutD3D9Meta);
			pOutD3D9Meta = NULL;
			FAIL;
		}
	}
}
//---------------------------------------------------------------------

DEM_DLL_EXPORT void DEM_DLLCALL FreeShaderMetadata(CSM30ShaderMeta* pD3D9Meta, CD3D11ShaderMeta* pD3D11Meta)
{
	Messages.Clear();

	if (pD3D9Meta) n_delete(pD3D9Meta);
	if (pD3D11Meta) n_delete(pD3D11Meta);
}
//---------------------------------------------------------------------

// Packs shaders in a sindle library file, big concatenated blob
// with a lookup table ID -> Offset. Returns packed shader count.
DEM_DLL_EXPORT unsigned int DEM_DLLCALL PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	Messages.Clear();

	// Get files by ID, include input signatures
	CString SQL("SELECT ID, Path, Size FROM Files WHERE ID IN(");
	SQL += pCommaSeparatedShaderIDs;
	SQL += ") OR ID IN (SELECT DISTINCT InputSigFileID FROM Shaders WHERE InputSigFileID <> 0 AND ObjFileID IN(";
	SQL += pCommaSeparatedShaderIDs;
	SQL += ")) ORDER BY ID ASC";

	DB::CValueTable Result;
	if (!ExecuteSQLQuery(SQL.CStr(), &Result)) return 0;
	if (!Result.GetRowCount()) return 0;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);

	IO::CFileStream File(pLibraryFilePath, FS);
	if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return 0;

	IO::CBinaryWriter W(File);

	W.Write<U32>('SLIB');				// Magic
	W.Write<U32>(0x0100);				// Version
	W.Write<U32>(Result.GetRowCount());	// Record count

	// Data starts after a header (12 bytes) and a lookup table
	// (4 bytes ID + 4 bytes offset + 4 bytes size for each record)
	UPTR CurrDataOffset = 12 + Result.GetRowCount() * 12;

	int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	int Col_Path = Result.GetColumnIndex(CStrID("Path"));
	int Col_Size = Result.GetColumnIndex(CStrID("Size"));

	// Write TOC, sorted by ID for faster lookup
	for (UPTR i = 0; i < Result.GetRowCount(); ++i)
	{
		U32 ID = (U32)Result.Get<int>(Col_ID, i);
		U32 Size = (U32)Result.Get<int>(Col_Size, i);

		W.Write<U32>(ID);
		W.Write<U32>(CurrDataOffset);
		W.Write<U32>(Size);

		CurrDataOffset += Size;
	}

	// Write binary data
	//???!!!how to preserve order passed when saving binary data?! is really critical?!

	for (UPTR i = 0; i < Result.GetRowCount(); ++i)
	{
		U32 Size = (U32)Result.Get<int>(Col_Size, i);
		const CString& Path = Result.Get<CString>(Col_Path, i);
		
		IO::CFileStream ObjFile(Path, FS);
		if (!ObjFile.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return i;

		n_assert(ObjFile.GetSize() == Size);

		//!!!can use mapped files instead!
		void* pData = n_malloc(Size);
		if (ObjFile.Read(pData, Size) != Size)
		{
			n_free(pData);
			return i;
		}
		if (File.Write(pData, Size) != Size)
		{
			n_free(pData);
			return i;
		}
		n_free(pData);
	}

	File.Close();

	return Result.GetRowCount();
}
//---------------------------------------------------------------------
