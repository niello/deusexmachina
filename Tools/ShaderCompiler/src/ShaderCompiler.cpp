#include "ShaderCompiler.h"
#include <Utils.h>
#include <DEMD3DInclude.h>
#include <ShaderDB.h>
#include <ShaderReflection.h>
#include <Render/USMShaderMeta.h>
#include <Render/SM30ShaderMeta.h>
#include <ValueTable.h>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

#undef CreateDirectory
#undef DeleteFile

bool ExtractUSMMetaFromBinary(const void* pData, size_t Size, CUSMShaderMeta& OutMeta, uint32_t& OutMinFeatureLevel, uint64_t& OutRequiresFlags, DEMShaderCompiler::ILogDelegate* pLog);
bool ExtractSM30MetaFromBinaryAndSource(CSM30ShaderMeta& OutMeta, const void* pData, size_t Size, const std::string& Source, DEMShaderCompiler::ILogDelegate* pLog);

struct CTargetParams
{
	const char*	pD3DTarget;
	uint32_t	FormatSignature;
	EShaderType ShaderType;
};

static bool FillTargetParams(EShaderType ShaderType, uint32_t Target, CTargetParams& Out)
{
	assert(Target <= 0x0500); // Add D3D12 'DXIL' format and sm5.1/6.0 targets

	const char* pTarget = nullptr;
	switch (ShaderType)
	{
		case ShaderType_Vertex:
		{
			switch (Target)
			{
				case 0x0500: pTarget = "vs_5_0"; break;
				case 0x0401: pTarget = "vs_4_1"; break;
				case 0x0400: pTarget = "vs_4_0"; break;
				case 0x0300: pTarget = "vs_3_0"; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Pixel:
		{
			switch (Target)
			{
				case 0x0500: pTarget = "ps_5_0"; break;
				case 0x0401: pTarget = "ps_4_1"; break;
				case 0x0400: pTarget = "ps_4_0"; break;
				case 0x0300: pTarget = "ps_3_0"; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Geometry:
		{
			switch (Target)
			{
				case 0x0500: pTarget = "gs_5_0"; break;
				case 0x0401: pTarget = "gs_4_1"; break;
				case 0x0400: pTarget = "gs_4_0"; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Hull:
		{
			switch (Target)
			{
				case 0x0500: pTarget = "hs_5_0"; break;
				default: return false;
			}
			break;
		}
		case ShaderType_Domain:
		{
			switch (Target)
			{
				case 0x0500: pTarget = "ds_5_0"; break;
				default: return false;
			}
			break;
		}
		default: return false;
	};

	Out.pD3DTarget = pTarget;
	Out.FormatSignature = (Target < 0x0400) ? 'DX9C' : 'DXBC';
	Out.ShaderType = ShaderType;

	return true;
}
//---------------------------------------------------------------------

// If base path is set, all other paths except absolute are relative to it, and all paths written to DB are relative to base too.
// If no base path is set (null or empty), all paths must be absolute or relative to CWD, and DB stores absolute paths.
// This allows to make content library movable and CWD-independent.
static void MakePaths(const char* pBasePath, const char* pPath, fs::path& OutFSPath, fs::path& OutDBPath)
{
	OutDBPath = fs::path(pPath).lexically_normal();
	OutFSPath = OutDBPath;
	if (OutDBPath.is_relative())
	{
		if (pBasePath)
			OutFSPath = fs::path(pBasePath) / OutFSPath;
		else
			OutDBPath = fs::weakly_canonical(OutDBPath);
	}
}
//---------------------------------------------------------------------

static int ProcessInputSignature(const char* pBasePath, const char* pInputSigDir, const char* pConfigName, ID3DBlob* pCode, DB::CShaderRecord& Rec)
{
	ID3DBlob* pInputSig = nullptr;
	if (FAILED(D3DGetInputSignatureBlob(pCode->GetBufferPointer(), pCode->GetBufferSize(), &pInputSig)))
	{
		// No input signature in a shader, this is OK
		Rec.InputSigFile.ID = 0;
		return DEM_SHADER_COMPILER_SUCCESS;
	}

	Rec.InputSigFile.Size = pInputSig->GetBufferSize();
	Rec.InputSigFile.CRC = CalcCRC((uint8_t*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());
	Rec.InputSigFile.ConfigName = pConfigName;

	// Try to find existing file in DB and on disk
	if (!DB::FindSignatureRecord(Rec.InputSigFile, pBasePath, pInputSig->GetBufferPointer()))
	{
		fs::path PathDB, PathFS;
		MakePaths(pBasePath, pInputSigDir, PathFS, PathDB);

		Rec.InputSigFile.Folder = PathDB.generic_string();

		if (!DB::WriteSignatureRecord(Rec.InputSigFile))
		{
			pInputSig->Release();
			return DEM_SHADER_COMPILER_DB_ERROR;
		}

		PathFS /= (std::to_string(Rec.InputSigFile.ID) + ".sig");

		fs::create_directories(PathFS.parent_path());

		std::ofstream File(PathFS, std::ios_base::binary | std::ios_base::trunc);
		if (!File || !File.write((const char*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize()))
		{
			pInputSig->Release();
			return DEM_SHADER_COMPILER_IO_WRITE_ERROR;
		}
	}

	pInputSig->Release();

	return DEM_SHADER_COMPILER_SUCCESS;
}
//---------------------------------------------------------------------

static int ProcessShaderBinaryUSM(const char* pBasePath, const char* pDestPath, ID3DBlob*& pCode, DB::CShaderRecord& Rec,
	const CTargetParams& TargetParams, bool Debug, DEMShaderCompiler::ILogDelegate* pLog)
{
	// Read metadata

	CUSMShaderMeta Meta;
	uint32_t MinFeatureLevel;
	uint64_t RequiresFlags;
	if (!ExtractUSMMetaFromBinary(pCode->GetBufferPointer(), pCode->GetBufferSize(), Meta, MinFeatureLevel, RequiresFlags, pLog))
		return DEM_SHADER_COMPILER_REFLECTION_ERROR;

	// Strip unnecessary info for release builds, making object files smaller

	if (!Debug)
	{
		// TODO: D3D12 - D3DCOMPILER_STRIP_ROOT_SIGNATURE ?
		ID3DBlob* pStrippedCode = nullptr;
		HRESULT hr = D3DStripShader(pCode->GetBufferPointer(), pCode->GetBufferSize(),
			D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
			&pStrippedCode);

		if (FAILED(hr))
		{
			if (pLog) pLog->LogError("\nD3DStripShader() failed\n");
			return DEM_SHADER_COMPILER_ERROR;
		}

		pCode->Release();
		pCode = pStrippedCode;
	}

	Rec.ObjFile.BytecodeSize = pCode->GetBufferSize();
	Rec.ObjFile.CRC = CalcCRC((const uint8_t*)pCode->GetBufferPointer(), pCode->GetBufferSize());

	/*
	// FIXME: this check prevents creation of the requested target file. Binary merging
	// must happen on packing, where different TOC records can resolve to the same chunk of bytes.
	// If object file is found, no additional actions are needed.
	// Compare only a shader blob for USM shaders, metadata completely depends on it.
	if (DB::FindBinaryRecord(Rec.ObjFile, pBasePath, pCode->GetBufferPointer(), true))
		return DEM_SHADER_COMPILER_SUCCESS;
	*/

	fs::path DestPathDB, DestPathFS;
	MakePaths(pBasePath, pDestPath, DestPathFS, DestPathDB);

	fs::create_directories(DestPathFS.parent_path());

	std::ofstream File(DestPathFS, std::ios_base::binary | std::ios_base::trunc);
	if (!File) return DEM_SHADER_COMPILER_IO_WRITE_ERROR;

	// Save common header

	WriteStream<uint32_t>(File, TargetParams.FormatSignature);
	WriteStream<uint32_t>(File, MinFeatureLevel);
	WriteStream<uint8_t>(File, TargetParams.ShaderType);

	// Metadata size for fast skipping
	const uint32_t MetaSize = GetSerializedSize(Meta) + sizeof(uint32_t) + sizeof(uint64_t);
	WriteStream<uint32_t>(File, MetaSize);

	// Save metadata

	const auto MetadataStart = File.tellp();

	WriteStream<uint32_t>(File, Rec.InputSigFile.ID);
	WriteStream<uint64_t>(File, RequiresFlags);

	File << Meta;

	assert(File.tellp() == MetadataStart + std::streampos(MetaSize));

	// Save shader binary
	File.write(static_cast<const char*>(pCode->GetBufferPointer()), pCode->GetBufferSize());

	Rec.ObjFile.Size = File.tellp();

	File.close();

	// Write binary file record to DB and obtain its ID
	// CRC is already calculated against the shader binary
	Rec.ObjFile.Path = DestPathDB.generic_string();
	return DB::WriteBinaryRecord(Rec.ObjFile) ? DEM_SHADER_COMPILER_SUCCESS : DEM_SHADER_COMPILER_DB_ERROR;
}
//---------------------------------------------------------------------

static int ProcessShaderBinarySM30(const char* pBasePath, const char* pDestPath, ID3DBlob* pCode, DB::CShaderRecord& Rec,
	CSM30ShaderMeta&& Meta, const CTargetParams& TargetParams, DEMShaderCompiler::ILogDelegate* pLog)
{
	// NB: D3DStripShader can't be applied to sm3.0 shaders

	Rec.ObjFile.BytecodeSize = pCode->GetBufferSize();
	Rec.ObjFile.CRC = CalcCRC((const uint8_t*)pCode->GetBufferPointer(), pCode->GetBufferSize());

	// Build binary from metadata + shader blob for binary comparison. Constant buffers are defined
	// in annotations and aren't reflected in a blob itself, so we must compare metadata too.

	const uint32_t MetaSize = GetSerializedSize(Meta);

	std::ostringstream ObjStream(std::ios_base::binary);
	ObjStream << Meta;

	assert(ObjStream.tellp() == MetaSize);

	ObjStream.write((const char*)pCode->GetBufferPointer(), pCode->GetBufferSize());

	/*
	// FIXME: this check prevents creation of the requested target file. Binary merging
	// must happen on packing, where different TOC records can resolve to the same chunk of bytes.
	// If object file is found, no additional actions are needed
	if (DB::FindBinaryRecord(Rec.ObjFile, pBasePath, ObjStream.str().c_str(), false))
		return DEM_SHADER_COMPILER_SUCCESS;
	*/

	fs::path DestPathDB, DestPathFS;
	MakePaths(pBasePath, pDestPath, DestPathFS, DestPathDB);

	fs::create_directories(DestPathFS.parent_path());

	std::ofstream File(DestPathFS, std::ios_base::binary | std::ios_base::trunc);
	if (!File) return DEM_SHADER_COMPILER_IO_WRITE_ERROR;

	// Save common header
	WriteStream<uint32_t>(File, TargetParams.FormatSignature);
	WriteStream<uint32_t>(File, GPU_Level_D3D9_3);
	WriteStream<uint8_t>(File, TargetParams.ShaderType);

	// Metadata size for fast skipping
	WriteStream<uint32_t>(File, MetaSize);

	// Metadata and bytecode are already serialized into memory, just save it
	const std::string ObjData = ObjStream.str();
	if (!ObjData.empty())
		File.write(ObjData.c_str(), ObjData.size());

	Rec.ObjFile.Size = File.tellp();

	File.close();

	// Write binary file record
	// CRC is already calculated against shader binary
	Rec.ObjFile.Path = DestPathDB.generic_string();
	return DB::WriteBinaryRecord(Rec.ObjFile) ? DEM_SHADER_COMPILER_SUCCESS : DEM_SHADER_COMPILER_DB_ERROR;
}
//---------------------------------------------------------------------

namespace DEMShaderCompiler
{

DEM_DLL_API bool DEM_DLLCALL Init(const char* pDBFileName)
{
	return DB::OpenConnection(pDBFileName);
}
//---------------------------------------------------------------------

DEM_DLL_API int DEM_DLLCALL CompileShader(const char* pBasePath, const char* pSrcPath, const char* pDestPath, const char* pInputSigDir,
	EShaderType ShaderType, uint32_t Target, const char* pConfigName, const char* pEntryPoint, const char* pDefines, bool Debug, bool Recompile,
	const char* pSrcData, size_t SrcDataSize, ILogDelegate* pLog)
{
	// Validate args

	if (!pSrcPath || !pDestPath || !pEntryPoint || ShaderType >= ShaderType_COUNT) return DEM_SHADER_COMPILER_INVALID_ARGS;

	if (!Target) Target = 0x0500;

	const bool HasInputSignature = (ShaderType == ShaderType_Vertex || ShaderType == ShaderType_Geometry) && Target >= 0x0400;
	if (HasInputSignature && !pInputSigDir) return DEM_SHADER_COMPILER_INVALID_ARGS;

	// Determine D3D target, output file extension and file signature

	CTargetParams TargetParams;
	if (!FillTargetParams(ShaderType, Target, TargetParams)) return DEM_SHADER_COMPILER_INVALID_ARGS;

	// Read the source file if not read yet

	fs::path SrcPathDB, SrcPathFS;
	MakePaths(pBasePath, pSrcPath, SrcPathFS, SrcPathDB);
	const std::string SrcPathFSStr = SrcPathFS.string();

	std::vector<char> SrcData;
	if (!pSrcData || !SrcDataSize)
	{
		if (!ReadAllFile(SrcPathFSStr.c_str(), SrcData)) return DEM_SHADER_COMPILER_IO_READ_ERROR;
		pSrcData = SrcData.data();
		SrcDataSize = SrcData.size();
	}

	// Build the compilation task description

	DB::CShaderRecord Rec;
	Rec.SrcFile.Path = SrcPathDB.generic_string();
	Rec.ShaderType = ShaderType;
	Rec.Target = Target;
	Rec.ConfigName = pConfigName;
	Rec.EntryPoint = pEntryPoint;

	// Parse preprocessor macro definitions

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

	std::vector<D3D_SHADER_MACRO> D3DMacros;
	if (Rec.Defines.size())
	{
		D3DMacros.reserve(Rec.Defines.size() + 1);

		for (const auto& Macro : Rec.Defines)
			D3DMacros.push_back({ Macro.first.c_str(), Macro.second.c_str() });

		// Terminating macro
		D3DMacros.push_back({ nullptr, nullptr });
	}
	const D3D_SHADER_MACRO* pD3DMacros = D3DMacros.empty() ? nullptr : D3DMacros.data();

	// Preprocess the source code

	//CDEMD3DInclude IncHandler(ExtractDirName(SrcPathFSStr), std::string{}); //RootPath);
	ID3DInclude* pInclude = D3D_COMPILE_STANDARD_FILE_INCLUDE;

	ID3DBlob* pCodeText = nullptr;
	ID3DBlob* pErrorMsgs = nullptr;
	HRESULT hr = D3DPreprocess(pSrcData, SrcDataSize, SrcPathFSStr.c_str(), pD3DMacros, pInclude, &pCodeText, &pErrorMsgs);

	if (FAILED(hr) || !pCodeText)
	{
		if (pLog) pLog->LogError(pErrorMsgs ? (const char*)pErrorMsgs->GetBufferPointer() : "<No D3D error message>");
		if (pCodeText) pCodeText->Release();
		if (pErrorMsgs) pErrorMsgs->Release();
		return false;
	}
	else if (pErrorMsgs)
	{
		if (pLog)
		{
			pLog->LogWarning("Preprocessed with warnings:\n");
			pLog->LogWarning((const char*)pErrorMsgs->GetBufferPointer());
		}
		pErrorMsgs->Release();
	}

	const std::string Source = static_cast<const char*>(pCodeText->GetBufferPointer());
	pCodeText->Release();

	// Setup compiler flags

	// D3DCOMPILE_IEEE_STRICTNESS
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL

	//!!!TODO: Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; // (more efficient, vec*mtx dots) //???does touch CPU-side const binding code?
	DWORD Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	if (Target >= 0x0400)
		Flags |= D3DCOMPILE_ENABLE_STRICTNESS; // Deny deprecated syntax

	if (Debug)
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL);
	else
		Flags |= (D3DCOMPILE_OPTIMIZATION_LEVEL3); // | D3DCOMPILE_SKIP_VALIDATION);

	// Check whether there were changes since the last compilation

	uint64_t CurrWriteTime = 0;
	{
		struct _stat FileStat;
		if (_stat(SrcPathFSStr.c_str(), &FileStat) == 0)
			CurrWriteTime = FileStat.st_mtime;
	}

	const size_t SrcCRC = CalcCRC(reinterpret_cast<const uint8_t*>(Source.c_str()), Source.size());

	bool Skip = true;
	std::string CompilationReason = "Compiling because:";
	if (Recompile)
	{
		Skip = false;
		CompilationReason += "\n - Forced recompliation requested";
	}
	else if (!DB::FindShaderRecord(Rec))
	{
		Skip = false;
		CompilationReason += "\n - Not found in cache DB";
	}
	else
	{
		if (Rec.CompilerVersion != D3D_COMPILER_VERSION)
		{
			Skip = false;
			CompilationReason += "\n - Compiler version changed";
		}
		if (Rec.CompilerFlags != Flags)
		{
			Skip = false;
			CompilationReason += "\n - Compiler flags changed";
		}
		if (Rec.SrcFile.Size != Source.size())
		{
			Skip = false;
			CompilationReason += "\n - Source size changed";
		}
		if (Rec.SrcFile.CRC != SrcCRC)
		{
			Skip = false;
			CompilationReason += "\n - Source CRC changed";
		}
		if (!CurrWriteTime || Rec.SrcModifyTimestamp != CurrWriteTime)
		{
			Skip = false;
			CompilationReason += "\n - Source file modification timestamp is newer or invalid";
		}
	}

	if (Skip)
	{
		if (pLog) pLog->LogInfo("No recompilation required, task skipped");
		return DEM_SHADER_COMPILER_UP_TO_DATE;
	}
	else
	{
		if (pLog) pLog->LogInfo(CompilationReason.c_str());
	}

	// We need to do the conversion, so update the description with current details

	// TODO: add our tool version too?
	Rec.CompilerVersion = D3D_COMPILER_VERSION;
	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = Source.size();
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Compile shader

	// There is also D3DCompile2 with 'secondary data' optional args but it is not useful for us now
	// TODO: RAII class for COM pointers? Would simplify the code.
	ID3DBlob* pCode = nullptr;
	hr = D3DCompile(Source.c_str(), Source.size(), SrcPathFSStr.c_str(), nullptr, nullptr,
		pEntryPoint, TargetParams.pD3DTarget, Flags, 0, &pCode, &pErrorMsgs);

	if (FAILED(hr) || !pCode)
	{
		if (pLog) pLog->LogError(pErrorMsgs ? (const char*)pErrorMsgs->GetBufferPointer() : "<No D3D error message>");
		if (pCode) pCode->Release();
		if (pErrorMsgs) pErrorMsgs->Release();
		return DEM_SHADER_COMPILER_COMPILE_ERROR;
	}
	else if (pErrorMsgs)
	{
		if (pLog)
		{
			pLog->LogWarning("Compiled with warnings:\n");
			pLog->LogWarning((const char*)pErrorMsgs->GetBufferPointer());
		}
		pErrorMsgs->Release();
	}

	// For vertex and geometry shaders, store input signature in a separate binary file.
	// It saves RAM since input signatures must reside in it at the runtime.

	const uint32_t OldSignatureID = Rec.InputSigFile.ID;

	if (HasInputSignature)
	{
		const auto ResultCode = ProcessInputSignature(pBasePath, pInputSigDir, pConfigName, pCode, Rec);
		if (ResultCode != DEM_SHADER_COMPILER_SUCCESS)
		{
			pCode->Release();
			return ResultCode;
		}
	}
	else Rec.InputSigFile.ID = 0;

	// Process shader binary

	const uint32_t OldBinaryID = Rec.ObjFile.ID;

	{
		int ResultCode;
		if (Target >= 0x0400)
		{
			// USM shaders store all necessary metadata in a shader blob itself
			ResultCode = ProcessShaderBinaryUSM(pBasePath, pDestPath, pCode, Rec, TargetParams, Debug, pLog);
		}
		else
		{
			// SM30 shaders can't rely on a shader blob only, because some metadata is stored in annotations
			CSM30ShaderMeta Meta;
			if (ExtractSM30MetaFromBinaryAndSource(Meta, pCode->GetBufferPointer(), pCode->GetBufferSize(), Source, pLog))
				ResultCode = ProcessShaderBinarySM30(pBasePath, pDestPath, pCode, Rec, std::move(Meta), TargetParams, pLog);
			else
				ResultCode = DEM_SHADER_COMPILER_REFLECTION_ERROR;
		}

		pCode->Release();

		if (ResultCode != DEM_SHADER_COMPILER_SUCCESS) return ResultCode;
	}

	// Write successfull compilation results to the database

	if (!DB::WriteShaderRecord(Rec)) return DEM_SHADER_COMPILER_DB_ERROR;

	// If resulting file IDs changed, the old ones might become unused and can be deleted

	if (OldSignatureID > 0 && OldSignatureID != Rec.InputSigFile.ID)
	{
		std::string OldPathStr;
		if (DB::ReleaseSignatureRecord(OldSignatureID, OldPathStr))
		{
			auto OldPath = fs::path(OldPathStr);
			if (pBasePath && OldPath.is_relative())
				OldPath = fs::path(pBasePath) / OldPath;
			fs::remove(OldPath);
		}
	}

	if (OldBinaryID > 0 && OldBinaryID != Rec.ObjFile.ID)
	{
		std::string OldPathStr;
		if (DB::ReleaseBinaryRecord(OldBinaryID, OldPathStr))
		{
			auto OldPath = fs::path(OldPathStr);
			if (pBasePath && OldPath.is_relative())
				OldPath = fs::path(pBasePath) / OldPath;
			fs::remove(OldPath);
		}
	}

	return DEM_SHADER_COMPILER_SUCCESS;
}
//---------------------------------------------------------------------

// Packs shaders in a sindle library file, big concatenated blob
// with a lookup table ID -> Offset. Returns packed shader count.
DEM_DLL_API uint32_t DEM_DLLCALL PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	// Get files by ID, include input signatures
	std::string SQL("SELECT ID, Path, Size FROM Files WHERE ID IN(");
	SQL += pCommaSeparatedShaderIDs;
	SQL += ") OR ID IN (SELECT DISTINCT InputSigFileID FROM Shaders WHERE InputSigFileID <> 0 AND BinaryFileID IN(";
	SQL += pCommaSeparatedShaderIDs;
	SQL += ")) ORDER BY ID ASC";

	CValueTable Result;
	if (!DB::ExecuteSQLQuery(SQL.c_str(), &Result)) return 0;
	if (!Result.GetRowCount()) return 0;

	std::ofstream File(pLibraryFilePath, std::ios_base::binary | std::ios_base::trunc);
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

		std::ifstream ObjFile(Path, std::ios_base::binary);
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
