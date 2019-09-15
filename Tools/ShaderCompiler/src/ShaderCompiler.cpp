#include "ShaderCompiler.h"

#include <Buffer.h>
#include <Data/StringUtils.h>
#include <IO/FS/FileSystemWin32.h>
#include <IO/Streams/FileStream.h>
#include <IO/Streams/MemStream.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/PathUtils.h>
#include <Util/UtilFwd.h>
#include <DEMD3DInclude.h>
#include <ShaderDB.h>
#include <ShaderReflectionSM30.h>
#include <ShaderReflectionUSM.h>
#include <ValueTable.h>

#undef CreateDirectory
#undef DeleteFile

std::string OutputDir;
std::string Messages;	// Last operation messages. Must be cleared at the start of every public API function.

struct CTargetParams
{
	const char*	pD3DTarget;
	const char*	pExtension;
	uint32_t	FileSignature;
};

class CAutoFree
{
private:

	void* pDataToFree;

public:

	CAutoFree(): pDataToFree(nullptr) {}
	~CAutoFree() { if (pDataToFree) free(pDataToFree); }

	void Set(void* pData) { pDataToFree = pData; }
};

DEM_DLL_API bool DEM_DLLCALL InitCompiler(const char* pDBFileName, const char* pOutputDirectory)
{
	Messages.clear();

	if (pOutputDirectory)
	{
		OutputDir.Set(pOutputDirectory);
		PathUtils::EnsurePathHasEndingDirSeparator(OutputDir);
	}
	else
	{
		OutputDir = PathUtils::ExtractDirName(pDBFileName);
		OutputDir += "../../Export/Shaders/Bin/";
		OutputDir = PathUtils::CollapseDots(OutputDir.c_str());
	}

	return OpenDB(pDBFileName);
}
//---------------------------------------------------------------------

DEM_DLL_API const char* DEM_DLLCALL GetLastOperationMessages()
{
	return Messages.c_str();
}
//---------------------------------------------------------------------

bool GetTargetParams(EShaderType ShaderType, uint32_t Target, CTargetParams& Out)
{
	const char* pTarget = nullptr;
	const char* pExt = nullptr;
	uint32_t FileSig;
	switch (ShaderType)
	{
		case ShaderType_Vertex:
		{
			pExt = "vsh";
			switch (Target)
			{
				case 0x0500: pTarget = "vs_5_0"; FileSig = 'VS50'; break;
				case 0x0401: pTarget = "vs_4_1"; FileSig = 'VS41'; break;
				case 0x0400: pTarget = "vs_4_0"; FileSig = 'VS40'; break;
				case 0x0300: pTarget = "vs_3_0"; FileSig = 'VS30'; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Pixel:
		{
			pExt = "psh";
			switch (Target)
			{
				case 0x0500: pTarget = "ps_5_0"; FileSig = 'PS50'; break;
				case 0x0401: pTarget = "ps_4_1"; FileSig = 'PS41'; break;
				case 0x0400: pTarget = "ps_4_0"; FileSig = 'PS40'; break;
				case 0x0300: pTarget = "ps_3_0"; FileSig = 'PS30'; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Geometry:
		{
			pExt = "gsh";
			switch (Target)
			{
				case 0x0500: pTarget = "gs_5_0"; FileSig = 'GS50'; break;
				case 0x0401: pTarget = "gs_4_1"; FileSig = 'GS41'; break;
				case 0x0400: pTarget = "gs_4_0"; FileSig = 'GS40'; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Hull:
		{
			pExt = "hsh";
			switch (Target)
			{
				case 0x0500: pTarget = "hs_5_0"; FileSig = 'HS50'; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Domain:
		{
			pExt = "dsh";
			switch (Target)
			{
				case 0x0500: pTarget = "ds_5_0"; FileSig = 'DS50'; break;
				default: return false;
			}
			break;
		}
		default: return false;
	};

	Out.pD3DTarget = pTarget;
	Out.pExtension = pExt;
	Out.FileSignature = FileSig;

	return true;
}
//---------------------------------------------------------------------

// String passed will be tokenized in place, so it must be read-write accessible.
// Memory management is a caller's responsibility.
bool ParseDefineString(char* pDefineString, std::vector<CMacroDBRec>& Out)
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
				Out.push_back(CurrMacro);
				pCurrDlms = pSemicolonOnly;
			}
			else // ';'
			{
				CurrMacro.Value = nullptr;
				if (!CurrMacro.Name)
				{
					CurrMacro.Name = pCurrPos;
					Out.push_back(CurrMacro);
				}
				CurrMacro.Name = nullptr;
				pCurrDlms = pBothDlms;
			}
			*pDlm = 0;
			pCurrPos = pDlm + 1;
		}
		else
		{
			CurrMacro.Value = nullptr;
			if (!CurrMacro.Name)
			{
				CurrMacro.Name = pCurrPos;
				Out.push_back(CurrMacro);
			}
			CurrMacro.Name = nullptr;
			break;
		}
	}

	// CurrMacro is both NULLs in all control pathes here, but we don't add it.
	// CompileShader() method takes care of it for D3D.

	return true;
}
//---------------------------------------------------------------------

// pDefines - "NAME[=VALUE];NAME[=VALUE];...NAME[=VALUE]"
DEM_DLL_API int DEM_DLLCALL CompileShader(const char* pSrcPath, EShaderType ShaderType, uint32_t Target, const char* pEntryPoint,
											 const char* pDefines, bool Debug, bool OnlyMetadata, uint32_t& ObjectFileID, uint32_t& InputSignatureFileID)
{
	Messages.clear();

	if (!pSrcPath || ShaderType >= ShaderType_COUNT || !pEntryPoint) return DEM_SHADER_COMPILER_INVALID_ARGS;

	if (!Target) Target = 0x0500;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);

	Data::CBuffer In;
	void* hFile = FS->OpenFile(pSrcPath, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!hFile) return DEM_SHADER_COMPILER_IO_READ_ERROR;
	uint64_t CurrWriteTime = FS->GetFileWriteTime(hFile);
	size_t FileSize = (size_t)FS->GetFileSize(hFile);
	In.Reserve(FileSize);
	size_t ReadSize = FS->Read(hFile, In.GetPtr(), FileSize);
	FS->CloseFile(hFile);
		
	if (ReadSize != FileSize) return DEM_SHADER_COMPILER_IO_READ_ERROR;

	size_t SrcCRC = Util::CalcCRC((uint8_t*)In.GetPtr(), In.GetSize());

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
		size_t DefinesLen = strlen(pDefines) + 1;
		pDefineStringBuffer = (char*)malloc(DefinesLen);
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

	std::vector<D3D_SHADER_MACRO> D3DMacros(Rec.Defines.size() + 1, 0);
	D3D_SHADER_MACRO* pD3DMacros;
	if (Rec.Defines.size())
	{
		for (size_t i = 0; i < Rec.Defines.size(); ++i)
		{
			const CMacroDBRec& Macro = Rec.Defines[i];
			D3D_SHADER_MACRO* pD3DMacro = D3DMacros.Add();
			pD3DMacro->Name = Macro.Name;
			pD3DMacro->Definition = Macro.Value;
		}

		// Terminating macro
		D3DMacros.push_back({ nullptr, nullptr });

		pD3DMacros = &D3DMacros[0];
	}
	else pD3DMacros = nullptr;

	// Compile shader

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(pSrcPath), std::string{}); //RootPath);

	ID3DBlob* pCode = nullptr;
	ID3DBlob* pErrors = nullptr;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), pSrcPath,
							pD3DMacros, &IncHandler, pEntryPoint, TargetParams.pD3DTarget,
							Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		Messages.assign(pErrors ? (const char*)pErrors->GetBufferPointer() : "<No D3D error message>");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		return DEM_SHADER_COMPILER_COMPILE_ERROR;
	}
	else if (pErrors)
	{
		Messages.assign("Compiled with warnings:\n\n");
		Messages += (const char*)pErrors->GetBufferPointer();
		Messages += '\n';
		pErrors->Release();
	}

	// For vertex and geometry shaders, store input signature in a separate binary file.
	// It saves RAM since input signatures must reside in it at the runtime.

	if (!OnlyMetadata && (ShaderType == ShaderType_Vertex || ShaderType == ShaderType_Geometry) && Target >= 0x0400)
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
			Rec.InputSigFile.CRC = Util::CalcCRC((uint8_t*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());

			uint32_t OldInputSigID = Rec.InputSigFile.ID;
			if (!FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer(), Target, Cmp_All))
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

				void* hFile = FS->OpenFile(Rec.InputSigFile.Path.c_str(), IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
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
				std::string OldObjPath;
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
	Rec.ObjFile.CRC = Util::CalcCRC((uint8_t*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	bool ObjFound;
	IO::PMemStream ObjStream;
	uint64_t BinaryOffset;
	if (Target < 0x0400)
	{
		// Collect and compare metadata for sm3.0 shaders. Binary file may differ even if a shader blob part
		// is the same, because constant buffers are defined in annotations and aren't reflected in a shader blob.
		ObjStream = n_new(IO::CMemStream);
		ObjStream->Open(IO::SAM_READWRITE);
		IO::CBinaryWriter W(*ObjStream.GetUnsafe());

		CSM30ShaderMeta Meta;
		bool MetaReflected = Meta.CollectFromBinaryAndSource(pCode->GetBufferPointer(), pCode->GetBufferSize(), (const char*)In.GetPtr(), In.GetSize(), IncHandler);
		bool MetaSaved = MetaReflected && Meta.Save(W);

		pCode->Release();
		
		if (!MetaReflected || !MetaSaved)
		{
			pFinalCode->Release();
			return MetaReflected ? DEM_SHADER_COMPILER_IO_WRITE_ERROR : DEM_SHADER_COMPILER_REFLECTION_ERROR;
		}

		if (OnlyMetadata)
		{
			BinaryOffset = 0; // Means that no binary shader bytecode included
		}
		else
		{
			BinaryOffset = ObjStream->GetPosition();
			ObjStream->Write(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());
		}

		pFinalCode->Release();

		ObjFound = FindObjFile(Rec.ObjFile, ObjStream->Map(), Target, Cmp_ShaderAndMetadata);
		ObjStream->Unmap();
	}
	else
	{
		// Compare only a shader blob for USM shaders
		ObjFound = FindObjFile(Rec.ObjFile, pFinalCode->GetBufferPointer(), Target, Cmp_Shader);
	}

	if (!ObjFound)
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

		W.Write<uint32_t>(TargetParams.FileSignature);

		// Offset of a shader binary, will fill later
		uint64_t OffsetOffset = File.GetPosition();
		W.Write<uint32_t>(0);

		W.Write<uint32_t>(Rec.ObjFile.ID);

		if (Target >= 0x0400)
		{
			W.Write<uint32_t>(Rec.InputSigFile.ID);
		}

		if (ObjStream.IsValidPtr())
		{
			// Data is already serialized into a memory, just save it
			BinaryOffset += File.GetPosition();
			File.Write(ObjStream->Map(), (size_t)ObjStream->GetSize());
			ObjStream->Unmap();
			ObjStream = nullptr;
		}
		else
		{
			n_assert(Target >= 0x0400);

			// Save metadata
			
			CUSMShaderMeta Meta;
			bool MetaReflected = Meta.CollectFromBinary(pCode->GetBufferPointer(), pCode->GetBufferSize());
			bool MetaSaved = MetaReflected && Meta.Save(W);

			pCode->Release();
		
			if (!MetaReflected || !MetaSaved)
			{
				pFinalCode->Release();
				return MetaReflected ? DEM_SHADER_COMPILER_IO_WRITE_ERROR : DEM_SHADER_COMPILER_REFLECTION_ERROR;
			}

			if (OnlyMetadata)
			{
				BinaryOffset = 0; // Means that no binary shader bytecode included
			}
			else
			{
				// Save shader binary
				BinaryOffset = File.GetPosition();
				File.Write(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());
			}

			pFinalCode->Release();
		}

		// Get total file size
		Rec.ObjFile.Size = File.GetPosition();

		if (BinaryOffset)
		{
			// Write binary data offset for fast skipping of metadata when reading
			File.Seek(OffsetOffset, IO::Seek_Begin);
			W.Write<uint32_t>((uint32_t)BinaryOffset);
		}

		File.Close();

		if (!UpdateObjFileRecord(Rec.ObjFile)) return DEM_SHADER_COMPILER_DB_ERROR;
	}
	else
	{
		// Object file found, no additional actions needed.
		// sm3.0 code has already released these buffers. 
		if (Target >= 0x0400)
		{
			pCode->Release();
			pFinalCode->Release();
		}
	}

	// If object file changed, remove reference to the old one
	// and delete it if no references left.
	if (OldObjFileID > 0 && OldObjFileID != Rec.ObjFile.ID)
	{
		std::string OldObjPath;
		if (ReleaseObjFile(OldObjFileID, OldObjPath))
			FS->DeleteFile(OldObjPath);
	}

	if (!WriteShaderRec(Rec)) return DEM_SHADER_COMPILER_DB_ERROR;

	ObjectFileID = Rec.ObjFile.ID;
	InputSignatureFileID = Rec.InputSigFile.ID;

	return DEM_SHADER_COMPILER_SUCCESS;
}
//---------------------------------------------------------------------

DEM_DLL_API void DEM_DLLCALL CreateShaderMetadata(EShaderModel ShaderModel, CShaderMetadata*& pOutMeta)
{
	if (ShaderModel == ShaderModel_30) pOutMeta = new CSM30ShaderMeta;
	else if (ShaderModel == ShaderModel_USM) pOutMeta = new CUSMShaderMeta;
	else pOutMeta = nullptr;
}
//---------------------------------------------------------------------

// Use FreeShaderMetadata() on the pointer returned.
DEM_DLL_API bool DEM_DLLCALL LoadShaderMetadataByObjectFileID(uint32_t ID, uint32_t& OutTarget, CShaderMetadata*& pOutMeta)
{
	Messages.clear();

	CObjFileData ObjFile;
	if (!FindObjFileByID(ID, ObjFile)) return false;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);
	IO::CFileStream File(ObjFile.Path.c_str(), FS);
	if (!File.Open(IO::SAM_READ)) return false;
	IO::CBinaryReader R(File);

	// File signature always stores target in bytes 1 and 0
	uint32_t FileSig;
	R.Read(FileSig);
	const char ByteHigh = (FileSig & 0x0000ff00) >> 8;
	const char ByteLow = (FileSig & 0x000000ff);
	OutTarget = ((ByteHigh - '0') << 8) | (ByteLow - '0');

	R.Read<uint32_t>();	// Binary data offset - skip
	R.Read<uint32_t>();	// Shader obj file ID - skip

	pOutMeta = nullptr;

	if (OutTarget >= 0x0400)
	{
		R.Read<uint32_t>();	// Input signature obj file ID - skip
		pOutMeta = new CUSMShaderMeta;
	}
	else pOutMeta = new CSM30ShaderMeta;

	if (!pOutMeta) return false;

	if (pOutMeta->Load(R)) return true;
	else
	{
		delete pOutMeta;
		pOutMeta = nullptr;
		return false;
	}
}
//---------------------------------------------------------------------

DEM_DLL_API void DEM_DLLCALL FreeShaderMetadata(CShaderMetadata* pMeta)
{
	Messages.clear();
	delete pMeta;
}
//---------------------------------------------------------------------

DEM_DLL_API bool DEM_DLLCALL SaveUSMShaderMetadata(IO::CBinaryWriter& W, const CShaderMetadata& Meta)
{
	Messages.clear();
	return Meta.Save(W);
}
//---------------------------------------------------------------------

// Packs shaders in a sindle library file, big concatenated blob
// with a lookup table ID -> Offset. Returns packed shader count.
DEM_DLL_API unsigned int DEM_DLLCALL PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	Messages.clear();

	// Get files by ID, include input signatures
	std::string SQL("SELECT ID, Path, Size FROM Files WHERE ID IN(");
	SQL += pCommaSeparatedShaderIDs;
	SQL += ") OR ID IN (SELECT DISTINCT InputSigFileID FROM Shaders WHERE InputSigFileID <> 0 AND ObjFileID IN(";
	SQL += pCommaSeparatedShaderIDs;
	SQL += ")) ORDER BY ID ASC";

	CValueTable Result;
	if (!ExecuteSQLQuery(SQL.c_str(), &Result)) return 0;
	if (!Result.GetRowCount()) return 0;

	IO::PFileSystem FS = n_new(IO::CFileSystemWin32);

	IO::CFileStream File(pLibraryFilePath, FS);
	if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return 0;

	IO::CBinaryWriter W(File);

	W.Write<uint32_t>('SLIB');				// Magic
	W.Write<uint32_t>(0x0100);				// Version
	W.Write<uint32_t>(Result.GetRowCount());	// Record count

	// Data starts after a header (12 bytes) and a lookup table
	// (4 bytes ID + 4 bytes offset + 4 bytes size for each record)
	size_t CurrDataOffset = 12 + Result.GetRowCount() * 12;

	int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	int Col_Path = Result.GetColumnIndex(CStrID("Path"));
	int Col_Size = Result.GetColumnIndex(CStrID("Size"));

	// Write TOC, sorted by ID for faster lookup
	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		uint32_t ID = (uint32_t)Result.Get<int>(Col_ID, i);
		uint32_t Size = (uint32_t)Result.Get<int>(Col_Size, i);

		W.Write<uint32_t>(ID);
		W.Write<uint32_t>(CurrDataOffset);
		W.Write<uint32_t>(Size);

		CurrDataOffset += Size;
	}

	// Write binary data
	//???!!!how to preserve order passed when saving binary data?! is really critical?!

	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		uint32_t Size = (uint32_t)Result.Get<int>(Col_Size, i);
		const std::string& Path = Result.Get<std::string>(Col_Path, i);
		
		IO::CFileStream ObjFile(Path, FS);
		if (!ObjFile.Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return i;

		n_assert(ObjFile.GetSize() == Size);

		//!!!can use mapped files instead!
		void* pData = malloc(Size);
		if (ObjFile.Read(pData, Size) != Size)
		{
			free(pData);
			return i;
		}
		if (File.Write(pData, Size) != Size)
		{
			free(pData);
			return i;
		}
		free(pData);
	}

	File.Close();

	return Result.GetRowCount();
}
//---------------------------------------------------------------------
