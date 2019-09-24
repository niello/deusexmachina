#include "ShaderCompiler.h"

#include <Utils.h>
#include <DEMD3DInclude.h>
#include <ShaderDB.h>
#include <ShaderReflectionSM30.h>
#include <ShaderReflectionUSM.h>
#include <ValueTable.h>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

#undef CreateDirectory
#undef DeleteFile

struct CTargetParams
{
	const char*	pD3DTarget;
	const char*	pExtension;
	uint32_t	FileSignature;
};

static bool GetTargetParams(EShaderType ShaderType, uint32_t Target, CTargetParams& Out)
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

static int ProcessInputSignature(const char* pInputSigDir, ID3DBlob* pCode, CShaderDBRec& Rec)
{
	ID3DBlob* pInputSig = nullptr;
	if (FAILED(D3DGetInputSignatureBlob(pCode->GetBufferPointer(), pCode->GetBufferSize(), &pInputSig)))
	{
		Rec.InputSigFile.ID = 0;
		return DEM_SHADER_COMPILER_SUCCESS;
	}

	Rec.InputSigFile.Size = pInputSig->GetBufferSize();
	Rec.InputSigFile.BytecodeSize = pInputSig->GetBufferSize();
	Rec.InputSigFile.CRC = CalcCRC((uint8_t*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());

	uint32_t OldInputSigID = Rec.InputSigFile.ID;
	if (!FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer(), Rec.Target, Cmp_All))
	{
		Rec.InputSigFile.ID = CreateObjFileRecord();
		if (!Rec.InputSigFile.ID)
		{
			pCode->Release();
			pInputSig->Release();
			return DEM_SHADER_COMPILER_DB_ERROR;
		}

		Rec.InputSigFile.Path = (fs::path(pInputSigDir) / std::to_string(Rec.InputSigFile.ID) / ".sig").lexically_normal().generic_string();

		EnsureDirectoryExists(ExtractDirName(Rec.InputSigFile.Path));

		{
			std::ofstream File(Rec.InputSigFile.Path.c_str());
			if (!File || !File.write((const char*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize()))
			{
				pCode->Release();
				pInputSig->Release();
				return DEM_SHADER_COMPILER_IO_WRITE_ERROR;
			}
			File.close();
		}

		if (!UpdateObjFileRecord(Rec.InputSigFile))
		{
			pCode->Release();
			pInputSig->Release();
			return DEM_SHADER_COMPILER_DB_ERROR;
		}
	}

	//!!!DUPLICATE CODE! SEE MAIN OBJ FILE BELOW!
	if (OldInputSigID > 0 && OldInputSigID != Rec.InputSigFile.ID)
	{
		std::string OldObjPath;
		if (ReleaseObjFile(OldInputSigID, OldObjPath))
			EraseFile(OldObjPath.c_str());
	}

	pInputSig->Release();

	return DEM_SHADER_COMPILER_SUCCESS;
}
//---------------------------------------------------------------------

namespace DEMShaderCompiler
{

DEM_DLL_API bool DEM_DLLCALL Init(const char* pDBFileName)
{
	return OpenDB(pDBFileName);
}
//---------------------------------------------------------------------

DEM_DLL_API int DEM_DLLCALL CompileShader(const char* pSrcPath, const char* pDestPath, const char* pInputSigDir,
	EShaderType ShaderType, uint32_t Target, const char* pEntryPoint, const char* pDefines, bool Debug,
	const char* pSrcData, size_t SrcDataSize, ILogDelegate* pLog)
{
	// Validate args

	if (!pSrcPath || !pDestPath || !pEntryPoint || ShaderType >= ShaderType_COUNT) return DEM_SHADER_COMPILER_INVALID_ARGS;

	if (!Target) Target = 0x0500;

	const bool HasInputSignature = (ShaderType == ShaderType_Vertex || ShaderType == ShaderType_Geometry) && Target >= 0x0400;
	if (HasInputSignature && !pInputSigDir) return DEM_SHADER_COMPILER_INVALID_ARGS;

	// Determine D3D target, output file extension and file signature

	CTargetParams TargetParams;
	if (!GetTargetParams(ShaderType, Target, TargetParams)) return DEM_SHADER_COMPILER_INVALID_ARGS;

	// Read the source file if not read yet

	std::vector<char> SrcData;
	if (!pSrcData || !SrcDataSize)
	{
		if (!ReadAllFile(pSrcPath, SrcData)) return DEM_SHADER_COMPILER_IO_READ_ERROR;
		pSrcData = SrcData.data();
		SrcDataSize = SrcData.size();
	}

	// Setup compiler flags

	// D3DCOMPILE_IEEE_STRICTNESS
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL

	//DWORD Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; // (more efficient, vec*mtx dots) //???does touch CPU-side const binding code?
	DWORD Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	if (Target >= 0x0400)
	{
		Flags |= D3DCOMPILE_ENABLE_STRICTNESS; // Deny deprecated syntax
	}

	if (Debug)
	{
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
	}
	else
	{
		Flags |= (D3DCOMPILE_OPTIMIZATION_LEVEL3); // | D3DCOMPILE_SKIP_VALIDATION);
	}

	// Build the compilation task description

	CShaderDBRec Rec;
	Rec.SrcFile.Path = pSrcPath;
	Rec.ShaderType = ShaderType;
	Rec.Target = Target;
	Rec.EntryPoint = pEntryPoint;

	// Parse define string

	if (pDefines)
	{
		auto Pairs = SplitString(pDefines, ';');
		for (auto& Pair : Pairs)
		{
			const auto Pos = Pair.find_first_of('=');
			if (Pos == std::string::npos)
				Rec.Defines.emplace(std::move(Pair), std::string{});
			else
				Rec.Defines.emplace(Pair.substr(0, Pos), Pair.substr(Pos + 1));
		}
	}

	// Check whether there were changes since the last conversion

	uint64_t CurrWriteTime = 0;
	{
		struct _stat FileStat;
		if (_stat(pSrcPath, &FileStat) == 0)
			CurrWriteTime = FileStat.st_mtime;
	}

	const size_t SrcCRC = CalcCRC((const uint8_t*)pSrcData, SrcDataSize);

	if (FindShaderRec(Rec) &&
		Rec.CompilerVersion == D3D_COMPILER_VERSION &&
		Rec.CompilerFlags == Flags &&
		Rec.SrcFile.Size == SrcDataSize &&
		Rec.SrcFile.CRC == SrcCRC &&
		CurrWriteTime &&
		Rec.SrcModifyTimestamp == CurrWriteTime)
	{
		return DEM_SHADER_COMPILER_SUCCESS;
	}

	// We need to do the conversion, so update the description with current details

	Rec.CompilerVersion = D3D_COMPILER_VERSION;
	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = SrcDataSize;
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Setup D3D shader macros

	std::vector<D3D_SHADER_MACRO> D3DMacros;
	D3D_SHADER_MACRO* pD3DMacros = nullptr;
	if (Rec.Defines.size())
	{
		D3DMacros.reserve(Rec.Defines.size() + 1);

		for (const auto& Macro : Rec.Defines)
			D3DMacros.push_back({ Macro.first.c_str(), Macro.second.c_str() });

		// Terminating macro
		D3DMacros.push_back({ nullptr, nullptr });

		pD3DMacros = &D3DMacros[0];
	}

	// Compile shader

	// TODO: try D3D_COMPILE_STANDARD_FILE_INCLUDE!
	//CDEMD3DInclude IncHandler(ExtractDirName(pSrcPath), std::string{}); //RootPath);
	ID3DInclude* pInclude = D3D_COMPILE_STANDARD_FILE_INCLUDE;

	// There is also D3DCompile2 with 'secondary data' optional args but it is not useful for us now
	// TODO: RAII class for COM pointers? Would simplify the code.
	ID3DBlob* pCode = nullptr;
	ID3DBlob* pErrorMsgs = nullptr;
	HRESULT hr = D3DCompile(pSrcData, SrcDataSize, pSrcPath,
		pD3DMacros, pInclude, pEntryPoint, TargetParams.pD3DTarget,
		Flags, 0, &pCode, &pErrorMsgs);

	if (FAILED(hr) || !pCode)
	{
		if (pLog) pLog->Log(pErrorMsgs ? (const char*)pErrorMsgs->GetBufferPointer() : "<No D3D error message>");
		if (pCode) pCode->Release();
		if (pErrorMsgs) pErrorMsgs->Release();
		return DEM_SHADER_COMPILER_COMPILE_ERROR;
	}
	else if (pErrorMsgs)
	{
		if (pLog)
		{
			pLog->Log("Compiled with warnings:\n");
			pLog->Log((const char*)pErrorMsgs->GetBufferPointer());
		}
		pErrorMsgs->Release();
	}

	// For vertex and geometry shaders, store input signature in a separate binary file.
	// It saves RAM since input signatures must reside in it at the runtime.

	if (HasInputSignature)
	{
		const auto ResultCode = ProcessInputSignature(pInputSigDir, pCode, Rec);
		if (ResultCode != DEM_SHADER_COMPILER_SUCCESS) return ResultCode;
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
		// TODO: D3D12 - D3DCOMPILER_STRIP_ROOT_SIGNATURE ?
		hr = D3DStripShader(pCode->GetBufferPointer(), pCode->GetBufferSize(),
			D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
			&pFinalCode);
		if (FAILED(hr))
		{
			if (pLog) pLog->Log("\nD3DStripShader() failed\n");
			pCode->Release();
			return DEM_SHADER_COMPILER_ERROR;
		}
	}

	Rec.ObjFile.BytecodeSize = pFinalCode->GetBufferSize();
	Rec.ObjFile.CRC = CalcCRC((const uint8_t*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	bool ObjFound;
	std::ostringstream ObjStream;
	uint64_t BinaryOffset;
	if (Target < 0x0400)
	{
		// Collect and compare metadata for sm3.0 shaders. Binary file may differ even if a shader blob part
		// is the same, because constant buffers are defined in annotations and aren't reflected in a shader blob.

		CSM30ShaderMeta Meta;
		const bool MetaReflected = Meta.CollectFromBinaryAndSource(pCode->GetBufferPointer(), pCode->GetBufferSize(), pSrcData, SrcDataSize, pInclude, pSrcPath, pD3DMacros, pLog);
		const bool MetaSaved = MetaReflected && Meta.Save(ObjStream);

		pCode->Release();

		if (!MetaReflected || !MetaSaved)
		{
			pFinalCode->Release();
			return MetaReflected ? DEM_SHADER_COMPILER_IO_WRITE_ERROR : DEM_SHADER_COMPILER_REFLECTION_ERROR;
		}

		BinaryOffset = ObjStream.tellp();
		ObjStream.write((const char*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

		pFinalCode->Release();

		ObjFound = FindObjFile(Rec.ObjFile, ObjStream.str().c_str(), Target, Cmp_ShaderAndMetadata);
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

		Rec.ObjFile.Path = pDestPath;

		EnsureDirectoryExists(ExtractDirName(Rec.ObjFile.Path));

		std::ofstream File(Rec.ObjFile.Path);
		if (!File)
		{
			pCode->Release();
			pFinalCode->Release();
			return DEM_SHADER_COMPILER_IO_WRITE_ERROR;
		}

		WriteStream<uint32_t>(File, TargetParams.FileSignature);

		// Offset of a shader binary, will fill later
		auto OffsetOffset = File.tellp();
		WriteStream<uint32_t>(File, 0);

		WriteStream<uint32_t>(File, Rec.ObjFile.ID);

		if (Target >= 0x0400)
		{
			WriteStream<uint32_t>(File, Rec.InputSigFile.ID);
		}

		const std::string ObjData = ObjStream.str();
		if (!ObjData.empty())
		{
			// Data is already serialized into a memory, just save it
			BinaryOffset += File.tellp();
			File.write(ObjData.c_str(), ObjData.size());
		}
		else
		{
			assert(Target >= 0x0400);

			// Save metadata

			CUSMShaderMeta Meta;
			bool MetaReflected = Meta.CollectFromBinary(pCode->GetBufferPointer(), pCode->GetBufferSize());
			bool MetaSaved = MetaReflected && Meta.Save(File);

			pCode->Release();

			if (!MetaReflected || !MetaSaved)
			{
				pFinalCode->Release();
				return MetaReflected ? DEM_SHADER_COMPILER_IO_WRITE_ERROR : DEM_SHADER_COMPILER_REFLECTION_ERROR;
			}

			// Save shader binary
			BinaryOffset = File.tellp();
			File.write((const char*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

			pFinalCode->Release();
		}

		// Get total file size
		Rec.ObjFile.Size = File.tellp();

		if (BinaryOffset)
		{
			// Write binary data offset for fast skipping of metadata when reading
			File.seekp(OffsetOffset, std::ios_base::beg);
			WriteStream<uint32_t>(File, static_cast<uint32_t>(BinaryOffset));
		}

		File.close();

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
			EraseFile(OldObjPath.c_str());
	}

	// Write successfull compilation results to the database

	if (!WriteShaderRec(Rec)) return DEM_SHADER_COMPILER_DB_ERROR;

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
	CObjFileData ObjFile;
	if (!FindObjFileByID(ID, ObjFile)) return false;

	std::ifstream File(ObjFile.Path.c_str());
	if (!File) return false;

	// File signature always stores target in bytes 1 and 0
	uint32_t FileSig;
	ReadStream(File, FileSig);
	const char ByteHigh = (FileSig & 0x0000ff00) >> 8;
	const char ByteLow = (FileSig & 0x000000ff);
	OutTarget = ((ByteHigh - '0') << 8) | (ByteLow - '0');

	ReadStream<uint32_t>(File);	// Binary data offset - skip
	ReadStream<uint32_t>(File);	// Shader obj file ID - skip

	pOutMeta = nullptr;

	if (OutTarget >= 0x0400)
	{
		ReadStream<uint32_t>(File);	// Input signature obj file ID - skip
		pOutMeta = new CUSMShaderMeta;
	}
	else pOutMeta = new CSM30ShaderMeta;

	if (!pOutMeta) return false;

	if (pOutMeta->Load(File)) return true;
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
	delete pMeta;
}
//---------------------------------------------------------------------

// Packs shaders in a sindle library file, big concatenated blob
// with a lookup table ID -> Offset. Returns packed shader count.
DEM_DLL_API uint32_t DEM_DLLCALL PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	// Get files by ID, include input signatures
	std::string SQL("SELECT ID, Path, Size FROM Files WHERE ID IN(");
	SQL += pCommaSeparatedShaderIDs;
	SQL += ") OR ID IN (SELECT DISTINCT InputSigFileID FROM Shaders WHERE InputSigFileID <> 0 AND ObjFileID IN(";
	SQL += pCommaSeparatedShaderIDs;
	SQL += ")) ORDER BY ID ASC";

	CValueTable Result;
	if (!ExecuteSQLQuery(SQL.c_str(), &Result)) return 0;
	if (!Result.GetRowCount()) return 0;

	std::ofstream File(pLibraryFilePath);
	if (!File) return 0;

	WriteStream<uint32_t>(File, 'SLIB');				// Magic
	WriteStream<uint32_t>(File, 0x0100);				// Version
	WriteStream<uint32_t>(File, Result.GetRowCount());	// Record count

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

		WriteStream<uint32_t>(File, ID);
		WriteStream<uint32_t>(File, CurrDataOffset);
		WriteStream<uint32_t>(File, Size);

		CurrDataOffset += Size;
	}

	// Write binary data
	//???!!!how to preserve order passed when saving binary data?! is really critical?!

	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		const size_t Size = static_cast<size_t>(Result.Get<int>(Col_Size, i));
		const std::string& Path = Result.Get<std::string>(Col_Path, i);

		std::ifstream ObjFile(Path);
		if (!ObjFile) return i;

		constexpr size_t BufferSize = 1024;
		char Buffer[1024];

		size_t RealSize = 0;
		while (File && ObjFile && !ObjFile.eof())
		{
			ObjFile.read(Buffer, BufferSize);
			File.write(Buffer, ObjFile.gcount());
			RealSize += static_cast<size_t>(ObjFile.gcount());
		}

		assert(RealSize == Size);

		ObjFile.close();
	}

	File.close();

	return Result.GetRowCount();
}
//---------------------------------------------------------------------

}