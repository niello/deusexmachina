#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>
#include <Util/UtilFwd.h> // CRC
#include <ToolRenderStateDesc.h>
#include <ConsoleApp.h>
#include <ShaderDB.h>
#include <D3D9ShaderReflection.h>
#include <DEMD3DInclude.h>
#include <D3DCompiler.inl>

#undef CreateDirectory
#undef DeleteFile

extern CString RootPath;

//???maybe pack shaders to some 'DB' file which maps DB ID to offset
//and reference shaders by DB ID, not by a file name?
//can even order shaders in memory and load one-by-one to minimize seek time
//may pack by use (common, menu, game, cinematic etc) and load only used.
//can store debug and release binary packages and read from what user wants

#define SRV_BUFFER 0xf000 // Highest bit of CVar::Register, SRV if set, CB if not

struct CD3D9ShaderBufferMeta
{
	CString			Name;
	CArray<UPTR>	UsedFloat4;
	CArray<UPTR>	UsedInt4;
	CArray<UPTR>	UsedBool;
};

struct CD3D9ShaderConstMeta
{
	CString			Name;
	ERegisterSet	RegSet; //???or type?
	U32				BufferIndex;
	U32				Offset;
	U32				Size;
	//type, array size
};

struct CD3D9ShaderRsrcMeta
{
	CString	TextureName;
	CString	SamplerName;
	U32		Register;
};

// For textures and samplers separately
//struct CD3D9ShaderRsrcMeta
//{
//	CString	Name;
//	U32		Registers;	// Bit field
//};

struct CD3D11ShaderBufferMeta
{
	CString	Name;
	U32		Register;
	U32		ElementSize;
	U32		ElementCount;
};

struct CD3D11ShaderConstMeta
{
	CString	Name;
	U32		BufferIndex;
	U32		Offset;
	U32		Size;
	//type, array size
};

struct CD3D11ShaderRsrcMeta
{
	CString	Name;
	U32		Register;
};

struct CTechInfo
{
	CStrID				ID;
	CStrID				InputSet;
	UPTR				MaxLights;
	CArray<CStrID>		Passes;
	CFixedArray<bool>	VariationValid;
	CFixedArray<UPTR>	PassIndices;
};

struct CRenderStateRef
{
	CStrID							ID;
	UPTR							MaxLights;
	Render::CToolRenderStateDesc	Desc;
	bool							UsesShader[Render::ShaderType_COUNT];
	U32*							ShaderIDs;	// Per shader type, per light count

	CRenderStateRef(): ShaderIDs(NULL) {}
	~CRenderStateRef() { if (ShaderIDs) n_free(ShaderIDs); }

	bool operator ==(const CRenderStateRef& Other) { return ID == Other.ID; }
};

void WriteRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
{
	U64 RangeCountOffset = W.GetStream().GetPosition();
	W.Write<U32>(0);

	if (!UsedRegs.GetCount()) return;

	U32 RangeCount = 0;
	UPTR CurrStart = UsedRegs[0], CurrCount = 1;
	for (UPTR r = 1; r < UsedRegs.GetCount(); ++r)
	{
		UPTR Reg = UsedRegs[r];
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			W.Write<U32>(CurrStart);
			W.Write<U32>(CurrCount);
			++RangeCount;
			n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
		n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
	}

	U64 EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<U32>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

//???!!!decouple DB and compilation?! to allow compilation without DB tracking, for CFD shaders like CEGUI.
int CompileShader(CShaderDBRec& Rec, bool Debug)
{
	CString SrcPath = RootPath + Rec.SrcFile.Path;
	Data::CBuffer In;
	if (!IOSrv->LoadFileToBuffer(SrcPath, In)) return ERR_IO_READ;
	U64 CurrWriteTime = IOSrv->GetFileWriteTime(SrcPath);
	UPTR SrcCRC = Util::CalcCRC((U8*)In.GetPtr(), In.GetSize());

	// Setup compiler flags

	// D3DCOMPILE_IEEE_STRICTNESS
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL

	//DWORD Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; // (more efficient, vec*mtx dots) //???does touch CPU-side const binding code?
	DWORD Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
	if (Rec.Target >= 0x0400)
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
		n_msg(VL_DETAILS, "Debug compilation on\n");
	}
	else Flags |= (D3DCOMPILE_OPTIMIZATION_LEVEL3); // | D3DCOMPILE_SKIP_VALIDATION);

	// Get info about previous compilation, skip if no changes

	Rec.SrcFile.Path = SrcPath;

	bool RecFound = FindShaderRec(Rec);
	if (RecFound &&
		Rec.CompilerFlags == Flags &&
		Rec.SrcFile.Size == In.GetSize() &&
		Rec.SrcFile.CRC == SrcCRC &&
		CurrWriteTime &&
		Rec.SrcModifyTimestamp == CurrWriteTime)
	{
		return SUCCESS;
	}

	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = In.GetSize();
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Determine D3D target, output file extension and file signature

	const char* pTarget = NULL;
	const char* pExt = NULL;
	Data::CFourCC FileSig;
	switch (Rec.ShaderType)
	{
		case Render::ShaderType_Vertex:
		{
			pExt = "vsh";
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "vs_5_0"; FileSig.Code = 'VS50'; break;
				case 0x0401: pTarget = "vs_4_1"; FileSig.Code = 'VS41'; break;
				case 0x0400: pTarget = "vs_4_0"; FileSig.Code = 'VS40'; break;
				case 0x0300: pTarget = "vs_3_0"; FileSig.Code = 'VS30'; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Pixel:
		{
			pExt = "psh";
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "ps_5_0"; FileSig.Code = 'PS50'; break;
				case 0x0401: pTarget = "ps_4_1"; FileSig.Code = 'PS41'; break;
				case 0x0400: pTarget = "ps_4_0"; FileSig.Code = 'PS40'; break;
				case 0x0300: pTarget = "ps_3_0"; FileSig.Code = 'PS30'; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Geometry:
		{
			pExt = "gsh";
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "gs_5_0"; FileSig.Code = 'GS50'; break;
				case 0x0401: pTarget = "gs_4_1"; FileSig.Code = 'GS41'; break;
				case 0x0400: pTarget = "gs_4_0"; FileSig.Code = 'GS40'; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Hull:
		{
			pExt = "hsh";
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "hs_5_0"; FileSig.Code = 'HS50'; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Domain:
		{
			pExt = "dsh";
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "ds_5_0"; FileSig.Code = 'DS50'; break;
				default: FAIL;
			}
			break;
		}
		default: FAIL;
	};

	// Setup shader macros

	CArray<D3D_SHADER_MACRO> Defines(8, 4);
	D3D_SHADER_MACRO* pDefines;
	if (Rec.Defines.GetCount())
	{
		for (UPTR i = 0; i < Rec.Defines.GetCount(); ++i)
		{
			const CMacroDBRec& Macro = Rec.Defines[i];
			D3D_SHADER_MACRO D3DMacro = { Macro.Name, Macro.Value };
			Defines.Add(D3DMacro);
		}

		// Terminating macro
		D3D_SHADER_MACRO D3DMacro = { NULL, NULL };
		Defines.Add(D3DMacro);

		pDefines = &Defines[0];
	}
	else pDefines = NULL;

	// Compile shader

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(Rec.SrcFile.Path), RootPath);

	ID3DBlob* pCode = NULL;
	ID3DBlob* pErrors = NULL;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), Rec.SrcFile.Path.CStr(),
							pDefines, &IncHandler, Rec.EntryPoint.CStr(), pTarget,
							Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n",
			Rec.ObjFile.Path.CStr(),
			pErrors ? pErrors->GetBufferPointer() : "No D3D error message.");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		return ERR_MAIN_FAILED;
	}
	else if (pErrors)
	{
		n_msg(VL_WARNING, "'%s' compiled with warnings:\n\n%s\n", Rec.ObjFile.Path.CStr(), pErrors->GetBufferPointer());
		pErrors->Release();
	}

	// For vertex shaders, store input signature in a separate binary file.
	// It saves RAM since input signatures must reside in it at the runtime.
	//???geometry shaders may have sig too?

	if (Rec.ShaderType == Render::ShaderType_Vertex && Rec.Target >= 0x0400)
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
			bool ObjFound = FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer(), false);
			if (!ObjFound)
			{
				if (!RegisterObjFile(Rec.InputSigFile, "sig")) // Fills empty ID and path inside
				{
					pCode->Release();
					pInputSig->Release();
					return ERR_MAIN_FAILED;
				}

				IOSrv->CreateDirectory(PathUtils::ExtractDirName(Rec.InputSigFile.Path));

				IO::CFileStream File(Rec.InputSigFile.Path);
				if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
				{
					pCode->Release();
					pInputSig->Release();
					return ERR_MAIN_FAILED;
				}
				File.Write(pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());
				File.Close();
			}

			if (OldInputSigID > 0 && OldInputSigID != Rec.InputSigFile.ID)
			{
				CString OldObjPath;
				if (ReleaseObjFile(OldInputSigID, OldObjPath))
					IOSrv->DeleteFile(OldObjPath);
			}

			pInputSig->Release();
		}
		else Rec.InputSigFile.ID = 0;
	}
	else Rec.InputSigFile.ID = 0;

	// Strip unnecessary info for release builds, making object files smaller

	ID3DBlob* pFinalCode;
	if (Debug || Rec.Target < 0x0400)
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
			return ERR_MAIN_FAILED;
		}
	}

	Rec.ObjFile.Size = pFinalCode->GetBufferSize();
	Rec.ObjFile.CRC = Util::CalcCRC((U8*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	bool ObjFound = FindObjFile(Rec.ObjFile, pFinalCode->GetBufferPointer(), true);
	if (!ObjFound)
	{
		if (!RegisterObjFile(Rec.ObjFile, pExt)) // Fills empty ID and path inside
		{
			pCode->Release();
			pFinalCode->Release();
			return ERR_MAIN_FAILED;
		}

		CString ShortPath(Rec.SrcFile.Path);
		ShortPath.Replace(IOSrv->ResolveAssigns("SrcShaders:") + "/", "SrcShaders:");
		n_msg(VL_DETAILS, "Shader: %s -> %s\n", ShortPath.CStr(), Rec.ObjFile.Path.CStr());

		IOSrv->CreateDirectory(PathUtils::ExtractDirName(Rec.ObjFile.Path));

		IO::CFileStream File(Rec.ObjFile.Path);
		if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			pFinalCode->Release();
			return ERR_MAIN_FAILED;
		}

		IO::CBinaryWriter W(File);

		W.Write(FileSig);

		// Offset of a shader binary, will fill later
		U64 OffsetOffset = File.GetPosition();
		W.Write<U32>(0);

		W.Write<U32>(Rec.ObjFile.ID);

		if (Rec.Target >= 0x0400)
		{
			W.Write<U32>(Rec.InputSigFile.ID);
		}

		// Save metadata

		if (Rec.Target >= 0x0400)
		{
			ID3D11ShaderReflection* pReflector = NULL;
			hr = D3D11Reflect(pCode->GetBufferPointer(), pCode->GetBufferSize(), &pReflector);

			pCode->Release();

			if (FAILED(hr))
			{
				n_msg(VL_ERROR, "	Shader reflection failed: '%s'\n", Rec.ObjFile.Path.CStr());
				pFinalCode->Release();
				return ERR_MAIN_FAILED;
			}

			//D3D_FEATURE_LEVEL FeatureLevel;
			//pReflector->GetMinFeatureLevel(&FeatureLevel);

			//pReflector->GetRequiresFlags();

			CArray<CD3D11ShaderConstMeta> Consts;
			CArray<CD3D11ShaderBufferMeta> Buffers;
			CArray<CD3D11ShaderRsrcMeta> Resources;
			CArray<CD3D11ShaderRsrcMeta> Samplers;

			D3D11_SHADER_DESC D3DDesc;
			pReflector->GetDesc(&D3DDesc);

			for (UINT RsrcIdx = 0; RsrcIdx < D3DDesc.BoundResources; ++RsrcIdx)
			{
				D3D11_SHADER_INPUT_BIND_DESC RsrcDesc;
				pReflector->GetResourceBindingDesc(RsrcIdx, &RsrcDesc);

				// D3D_SIF_USERPACKED - may fail assertion if not set!

				switch (RsrcDesc.Type)
				{
					case D3D_SIT_TEXTURE:
					{
						n_msg(VL_DETAILS, "  Texture: %s, %d slot(s) from %d\n", RsrcDesc.Name, RsrcDesc.BindCount, RsrcDesc.BindPoint);
						n_assert(RsrcDesc.BindCount == 1);
						CD3D11ShaderRsrcMeta* pMeta = Resources.Reserve(1);
						pMeta->Name = RsrcDesc.Name;
						pMeta->Register = RsrcDesc.BindPoint;
						break;
					}
					case D3D_SIT_SAMPLER:
					{
						n_msg(VL_DETAILS, "  Sampler: %s, %d slot(s) from %d\n", RsrcDesc.Name, RsrcDesc.BindCount, RsrcDesc.BindPoint);
						// D3D_SIF_COMPARISON_SAMPLER
						n_assert(RsrcDesc.BindCount == 1);
						CD3D11ShaderRsrcMeta* pMeta = Samplers.Reserve(1);
						pMeta->Name = RsrcDesc.Name;
						pMeta->Register = RsrcDesc.BindPoint;
						break;
					}
					case D3D_SIT_CBUFFER:
					case D3D_SIT_TBUFFER: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
					case D3D_SIT_STRUCTURED: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
					{
						n_msg(VL_DETAILS, "  CBuffer: %s, %d slot(s) from %d\n", RsrcDesc.Name, RsrcDesc.BindCount, RsrcDesc.BindPoint);
						n_assert(RsrcDesc.BindCount == 1);

						ID3D11ShaderReflectionConstantBuffer* pCB = pReflector->GetConstantBufferByName(RsrcDesc.Name);
						if (!pCB) continue;

						D3D11_SHADER_BUFFER_DESC D3DBufDesc;
						pCB->GetDesc(&D3DBufDesc);
						if (!D3DBufDesc.Variables) continue;

						DWORD TypeMask;
						if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = (1 << 30);
						else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = (2 << 30);
						else TypeMask = 0;

						n_assert_dbg(!TypeMask);

						CD3D11ShaderBufferMeta* pMeta = Buffers.Reserve(1);
						pMeta->Name = RsrcDesc.Name;
						pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
						pMeta->ElementSize = D3DBufDesc.Size;
						pMeta->ElementCount = 1;

						for (UINT VarIdx = 0; VarIdx < D3DBufDesc.Variables; ++VarIdx)
						{
							ID3D11ShaderReflectionVariable* pVar = pCB->GetVariableByIndex(VarIdx);
							if (!pVar) continue;

							D3D11_SHADER_VARIABLE_DESC D3DVarDesc;
							pVar->GetDesc(&D3DVarDesc);

							//D3D_SVF_USERPACKED             = 1,
							//D3D_SVF_USED                   = 2,

							ID3D11ShaderReflectionType* pVarType = pVar->GetType();
							if (!pVarType) continue;

							D3D11_SHADER_TYPE_DESC D3DTypeDesc;
							pVarType->GetDesc(&D3DTypeDesc);

							n_msg(VL_DETAILS, "    Const: %s, offset %d, size %d, type %d\n",
								  D3DVarDesc.Name, D3DVarDesc.StartOffset, D3DVarDesc.Size, D3DTypeDesc.Type);

							CD3D11ShaderConstMeta* pMeta = Consts.Reserve(1);
							pMeta->Name = D3DVarDesc.Name;
							pMeta->BufferIndex = Buffers.GetCount() - 1;
							pMeta->Offset = D3DVarDesc.StartOffset;
							pMeta->Size = D3DVarDesc.Size;

							//D3D11_SHADER_TYPE_DESC
						}

						break;
					}
				}
			}

			pReflector->Release();

			W.Write<U32>(Buffers.GetCount());
			for (UPTR i = 0; i < Buffers.GetCount(); ++i)
			{
				const CD3D11ShaderBufferMeta& B = Buffers[i];
				W.Write(B.Name);
				W.Write(B.Register);
				W.Write(B.ElementSize);
				W.Write(B.ElementCount);
			}

			W.Write<U32>(Consts.GetCount());
			for (UPTR i = 0; i < Consts.GetCount(); ++i)
			{
				const CD3D11ShaderConstMeta& B = Consts[i];
				W.Write(B.Name);
				W.Write(B.BufferIndex);
				W.Write(B.Offset);
				W.Write(B.Size);
			}

			W.Write<U32>(Resources.GetCount());
			for (UPTR i = 0; i < Resources.GetCount(); ++i)
			{
				const CD3D11ShaderRsrcMeta& B = Resources[i];
				W.Write(B.Name);
				W.Write(B.Register);
			}

			W.Write<U32>(Samplers.GetCount());
			for (UPTR i = 0; i < Samplers.GetCount(); ++i)
			{
				const CD3D11ShaderRsrcMeta& B = Samplers[i];
				W.Write(B.Name);
				W.Write(B.Register);
			}
		}
		else
		{
			pCode->Release();

			CArray<CD3D9ConstantDesc> D3D9Consts;
			CString Creator;

			if (!D3D9Reflect(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize(), D3D9Consts, Creator))
			{
				n_msg(VL_ERROR, "	Shader reflection failed: '%s'\n", Rec.ObjFile.Path.CStr());
				pFinalCode->Release();
				return ERR_MAIN_FAILED;
			}

			CDict<CString, CString> SampToTex;
			D3D9FindSamplerTextures((const char*)In.GetPtr(), In.GetSize(), SampToTex);

			CArray<CD3D9ShaderConstMeta> Consts;
			CArray<CD3D9ShaderBufferMeta> Buffers;
			CArray<CD3D9ShaderRsrcMeta> Samplers;

			CD3D9ShaderBufferMeta* pMeta = Buffers.Reserve(1);
			pMeta->Name = "$Global";

			for (UPTR i = 0; i < D3D9Consts.GetCount(); ++i)
			{
				CD3D9ConstantDesc& D3D9ConstDesc = D3D9Consts[i];

				if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
				{
					CD3D9ShaderRsrcMeta* pMeta = Samplers.Reserve(1);
					pMeta->SamplerName = D3D9ConstDesc.Name;
					pMeta->Register = D3D9ConstDesc.RegisterIndex; //???support sampler arrays?

					int STIdx = SampToTex.FindIndex(D3D9ConstDesc.Name);
					if (STIdx != INVALID_INDEX) pMeta->TextureName = SampToTex.ValueAt(STIdx);
				}
				else
				{
					CD3D9ShaderConstMeta* pMeta = Consts.Reserve(1);
					pMeta->Name = D3D9ConstDesc.Name;
					pMeta->RegSet = D3D9ConstDesc.RegisterSet;
					pMeta->Offset = D3D9ConstDesc.RegisterIndex;
					pMeta->Size = D3D9ConstDesc.RegisterCount;

					// Try to get owning pseudo-buffer from extra info, else use default

					pMeta->BufferIndex = 0; // Default, global buffer

					CD3D9ShaderBufferMeta& BufMeta = Buffers[pMeta->BufferIndex];
					CArray<UPTR>& UsedRegs = (pMeta->RegSet == RS_FLOAT4) ? BufMeta.UsedFloat4 : ((pMeta->RegSet == RS_INT4) ? BufMeta.UsedInt4 : BufMeta.UsedBool);
					for (UPTR r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
					{
						if (!UsedRegs.Contains(r)) UsedRegs.Add(r);
					}
				}
			}

			UPTR i = 0;
			while (i < Buffers.GetCount())
			{
				CD3D9ShaderBufferMeta& B = Buffers[i];
				if (!B.UsedFloat4.GetCount() &&
					!B.UsedInt4.GetCount() &&
					!B.UsedBool.GetCount())
				{
					Buffers.RemoveAt(i);
				}
				else ++i;
			};

			W.Write<U32>(Buffers.GetCount());
			for (UPTR i = 0; i < Buffers.GetCount(); ++i)
			{
				CD3D9ShaderBufferMeta& B = Buffers[i];
				B.UsedFloat4.Sort();
				B.UsedInt4.Sort();
				B.UsedBool.Sort();

				n_msg(VL_DETAILS, "  CBuffer: %s\n", B.Name.CStr());

				W.Write(B.Name);

				WriteRegisterRanges(B.UsedFloat4, W, "float4");
				WriteRegisterRanges(B.UsedInt4, W, "int4");
				WriteRegisterRanges(B.UsedBool, W, "bool");
			}

			W.Write<U32>(Consts.GetCount());
			for (UPTR i = 0; i < Consts.GetCount(); ++i)
			{
				const CD3D9ShaderConstMeta& Meta = Consts[i];

				U8 RegSet;
				switch (Meta.RegSet)
				{
					case RS_FLOAT4:	RegSet = 0; break;
					case RS_INT4:	RegSet = 1; break;
					case RS_BOOL:	RegSet = 2; break;
					default:		continue;
				};

				W.Write(Meta.Name);
				W.Write(Meta.BufferIndex);
				W.Write<U8>(RegSet);
				W.Write(Meta.Offset);
				W.Write(Meta.Size);

				const char* pRegisterSetName = NULL;
				switch (Meta.RegSet)
				{
					case RS_FLOAT4:	pRegisterSetName = "float4"; break;
					case RS_INT4:	pRegisterSetName = "int4"; break;
					case RS_BOOL:	pRegisterSetName = "bool"; break;
				};
				n_msg(VL_DETAILS, "  Const: %s, %s %d to %d\n",
						Meta.Name.CStr(), pRegisterSetName, Meta.Offset, Meta.Offset + Meta.Size - 1);
			}

			W.Write<U32>(Samplers.GetCount());
			for (UPTR i = 0; i < Samplers.GetCount(); ++i)
			{
				const CD3D9ShaderRsrcMeta& Meta = Samplers[i];
				W.Write(Meta.SamplerName);
				W.Write(Meta.TextureName);
				W.Write(Meta.Register);		//!!!need sampler arrays! need one texture to multiple samplers!

				n_msg(VL_DETAILS, "  Sampler: %s (texture %s), %d slot(s) from %d\n", Meta.SamplerName.CStr(), Meta.TextureName.CStr(), 1, Meta.Register);
			}
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
			IOSrv->DeleteFile(OldObjPath);
	}

	return WriteShaderRec(Rec) ? SUCCESS : ERR_MAIN_FAILED;
}
//---------------------------------------------------------------------

Render::ECmpFunc StringToCmpFunc(const CString& Str)
{
	if (Str == "less" || Str == "l") return Render::Cmp_Less;
	if (Str == "lessequal" || Str == "le") return Render::Cmp_LessEqual;
	if (Str == "greater" || Str == "g") return Render::Cmp_Greater;
	if (Str == "greaterequal" || Str == "ge") return Render::Cmp_GreaterEqual;
	if (Str == "equal" || Str == "e") return Render::Cmp_Equal;
	if (Str == "notequal" || Str == "ne") return Render::Cmp_NotEqual;
	if (Str == "always") return Render::Cmp_Always;
	return Render::Cmp_Never;
}
//---------------------------------------------------------------------

Render::EStencilOp StringToStencilOp(const CString& Str)
{
	if (Str == "zero") return Render::StencilOp_Zero;
	if (Str == "replace") return Render::StencilOp_Replace;
	if (Str == "inc") return Render::StencilOp_Inc;
	if (Str == "incsat") return Render::StencilOp_IncSat;
	if (Str == "dec") return Render::StencilOp_Dec;
	if (Str == "decsat") return Render::StencilOp_DecSat;
	if (Str == "invert") return Render::StencilOp_Invert;
	return Render::StencilOp_Keep;
}
//---------------------------------------------------------------------

Render::EBlendArg StringToBlendArg(const CString& Str)
{
	if (Str == "one") return Render::BlendArg_One;
	if (Str == "srccolor") return Render::BlendArg_SrcColor;
	if (Str == "invsrccolor") return Render::BlendArg_InvSrcColor;
	if (Str == "src1color") return Render::BlendArg_Src1Color;
	if (Str == "invsrc1color") return Render::BlendArg_InvSrc1Color;
	if (Str == "srcalpha") return Render::BlendArg_SrcAlpha;
	if (Str == "srcalphasat") return Render::BlendArg_SrcAlphaSat;
	if (Str == "invsrcalpha") return Render::BlendArg_InvSrcAlpha;
	if (Str == "src1alpha") return Render::BlendArg_Src1Alpha;
	if (Str == "invsrc1alpha") return Render::BlendArg_InvSrc1Alpha;
	if (Str == "destcolor") return Render::BlendArg_DestColor;
	if (Str == "invdestcolor") return Render::BlendArg_InvDestColor;
	if (Str == "destalpha") return Render::BlendArg_DestAlpha;
	if (Str == "invdestalpha") return Render::BlendArg_InvDestAlpha;
	if (Str == "blendfactor") return Render::BlendArg_BlendFactor;
	if (Str == "invblendfactor") return Render::BlendArg_InvBlendFactor;
	return Render::BlendArg_Zero;
}
//---------------------------------------------------------------------

Render::EBlendOp StringToBlendOp(const CString& Str)
{
	if (Str == "sub") return Render::BlendOp_Sub;
	if (Str == "revsub") return Render::BlendOp_RevSub;
	if (Str == "min") return Render::BlendOp_Min;
	if (Str == "max") return Render::BlendOp_Max;
	return Render::BlendOp_Add;
}
//---------------------------------------------------------------------

bool ProcessShaderSection(Data::PParams ShaderSection, Render::EShaderType ShaderType, bool Debug, CRenderStateRef& RSRef)
{
	UPTR LightVariationCount = RSRef.MaxLights + 1;

	// Set invalid shader ID (no shader)
	for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] = 0;

	RSRef.UsesShader[ShaderType] = ShaderSection.IsValidPtr();
	if (!RSRef.UsesShader[ShaderType]) OK;

	CString SrcPath;
	ShaderSection->Get(SrcPath, CStrID("In"));
	CString EntryPoint;
	ShaderSection->Get(EntryPoint, CStrID("Entry"));
	int Target = 0;
	ShaderSection->Get(Target, CStrID("Target"));

	SrcPath = IOSrv->ResolveAssigns(SrcPath);
	SrcPath.Replace(RootPath, "");

	CString Defines;
	char* pDefineString = NULL;
	CArray<CMacroDBRec> Macros;
	if (ShaderSection->Get(Defines, CStrID("Defines"))) // NAME[=VALUE];NAME[=VALUE];...NAME[=VALUE]
	{
		Defines.Trim();

		if (Defines.GetLength())
		{
			pDefineString = (char*)n_malloc(Defines.GetLength() + 1);
			strcpy_s(pDefineString, Defines.GetLength() + 1, Defines.CStr());

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
						Macros.Add(CurrMacro);
						pCurrDlms = pSemicolonOnly;
					}
					else // ';'
					{
						CurrMacro.Value = NULL;
						if (!CurrMacro.Name)
						{
							CurrMacro.Name = pCurrPos;
							Macros.Add(CurrMacro);
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
						Macros.Add(CurrMacro);
					}
					CurrMacro.Name = NULL;
					break;
				}
			}

			// CurrMacro is both NULLs in all control pathes here, but we don't add it.
			// CompileShader() method takes care of it.
		}
	}

	for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
	{		
		CShaderDBRec Rec;
		Rec.ShaderType = ShaderType;
		Rec.SrcFile.Path = SrcPath;
		Rec.EntryPoint = EntryPoint;
		Rec.Target = Target;
		Rec.Defines = Macros;

		CString StrLightCount = StringUtils::FromInt(LightCount);
		CMacroDBRec LightMacro;
		LightMacro.Name = "DEM_LIGHT_COUNT";
		LightMacro.Value = StrLightCount.CStr();
		Rec.Defines.Add(LightMacro);

		int Result = CompileShader(Rec, Debug);
		RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] = (Result == SUCCESS) ? Rec.ObjFile.ID : 0;
	}

	if (pDefineString) n_free(pDefineString);

	OK;
}
//---------------------------------------------------------------------

bool ProcessBlendSection(Data::PParams BlendSection, int Index, Render::CToolRenderStateDesc& Desc)
{
	if (Index < 0 || Index > 7) FAIL;

	CString StrValue;
	bool FlagValue;

	if (BlendSection->Get(FlagValue, CStrID("Enable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Blend_RTBlendEnable << Index, FlagValue);

	Render::CToolRenderStateDesc::CRTBlend& Blend = Desc.RTBlend[Index];

	if (BlendSection->Get(StrValue, CStrID("SrcBlendArg")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.SrcBlendArg = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("SrcBlendArgAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.SrcBlendArgAlpha = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("DestBlendArg")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.DestBlendArg = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("DestBlendArgAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.DestBlendArgAlpha = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("BlendOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.BlendOp = StringToBlendOp(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("BlendOpAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.BlendOpAlpha = StringToBlendOp(StrValue);
	}

	if (BlendSection->Get(StrValue, CStrID("WriteMask")))
	{
		unsigned char Mask = 0;
		StrValue.ToLower();
		if (StrValue.FindIndex('r') != INVALID_INDEX) Mask |= Render::ColorMask_Red;
		if (StrValue.FindIndex('g') != INVALID_INDEX) Mask |= Render::ColorMask_Green;
		if (StrValue.FindIndex('b') != INVALID_INDEX) Mask |= Render::ColorMask_Blue;
		if (StrValue.FindIndex('a') != INVALID_INDEX) Mask |= Render::ColorMask_Alpha;
		Blend.WriteMask = Mask;
	}

	OK;
}
//---------------------------------------------------------------------

bool ReadRenderStateDesc(Data::PParams RenderStates, CStrID ID, Render::CToolRenderStateDesc& Desc, Data::PParams* ShaderSections)
{
	Data::PParams RS;
	if (!RenderStates->Get(RS, ID)) FAIL;

	//!!!may store already loaded in cache to avoid unnecessary reloading!
	Data::CParam* pPrmBaseID;
	if (RS->Get(pPrmBaseID, CStrID("Base")))
	{
		if (!ReadRenderStateDesc(RenderStates, pPrmBaseID->GetValue<CStrID>(), Desc, ShaderSections)) FAIL;
	}

	// Shaders

	const char* pShaderSectionName[] = { "VS", "PS", "GS", "HS", "DS" };

	Data::PParams ShaderSection;
	for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
	{
		if (RS->Get(ShaderSection, CStrID(pShaderSectionName[ShaderType])))
		{
			if (!ShaderSections) return ERR_MAIN_FAILED;
			Data::PParams CurrShaderSection = ShaderSections[ShaderType];
			if (CurrShaderSection.IsValidPtr() && CurrShaderSection->GetCount())
				CurrShaderSection->Merge(*ShaderSection, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep);
			else
				ShaderSections[ShaderType] = ShaderSection;
		}
	}

	// States

	CString StrValue;
	int IntValue;
	bool FlagValue;
	vector4 Vector4Value;

	// Rasterizer

	if (RS->Get(StrValue, CStrID("Cull")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		if (StrValue == "none")
		{
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else if (StrValue == "front")
		{
			Desc.Flags.Set(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else if (StrValue == "back")
		{
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Set(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else
		{
			n_msg(VL_ERROR, "Unrecognized 'Cull' value: %s\n", StrValue.CStr());
			FAIL;
		}
	}

	if (RS->Get(FlagValue, CStrID("Wireframe")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_Wireframe, FlagValue);
	if (RS->Get(FlagValue, CStrID("FrontCCW")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_FrontCCW, FlagValue);
	if (RS->Get(FlagValue, CStrID("DepthClipEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_DepthClipEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("ScissorEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_ScissorEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("MSAAEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_MSAAEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("MSAALinesEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_MSAALinesEnable, FlagValue);

	RS->Get(Desc.DepthBias, CStrID("DepthBias"));
	RS->Get(Desc.DepthBiasClamp, CStrID("DepthBiasClamp"));
	RS->Get(Desc.SlopeScaledDepthBias, CStrID("SlopeScaledDepthBias"));

	// Depth-stencil

	if (RS->Get(FlagValue, CStrID("DepthEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_DepthEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("DepthWriteEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_DepthWriteEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("StencilEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_StencilEnable, FlagValue);

	if (RS->Get(StrValue, CStrID("DepthFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.DepthFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(IntValue, CStrID("StencilReadMask"))) Desc.StencilReadMask = IntValue;
	if (RS->Get(IntValue, CStrID("StencilWriteMask"))) Desc.StencilWriteMask = IntValue;
	if (RS->Get(StrValue, CStrID("StencilFrontFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontPassOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilPassOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontDepthFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilDepthFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackPassOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilPassOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackDepthFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilDepthFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(IntValue, CStrID("StencilRef"))) Desc.StencilRef = IntValue;

	// Blend

	if (RS->Get(FlagValue, CStrID("AlphaToCoverage")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Blend_AlphaToCoverage, FlagValue);

	if (RS->Get(Vector4Value, CStrID("BlendFactor")))
		memcpy(Desc.BlendFactorRGBA, Vector4Value.v, sizeof(float) * 4);
	if (RS->Get(IntValue, CStrID("SampleMask"))) Desc.SampleMask = IntValue;

	Data::CParam* pBlend;
	if (RS->Get(pBlend, CStrID("Blend")))
	{
		if (pBlend->IsA<Data::PParams>())
		{
			Data::PParams BlendSection = pBlend->GetValue<Data::PParams>();
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Blend_Independent);
			if (!ProcessBlendSection(BlendSection, 0, Desc)) FAIL;
		}
		else if (pBlend->IsA<Data::PDataArray>())
		{
			Data::PDataArray BlendArray = pBlend->GetValue<Data::PDataArray>();
			Desc.Flags.Set(Render::CToolRenderStateDesc::Blend_Independent);
			for (UPTR BlendIdx = 0; BlendIdx < BlendArray->GetCount(); ++BlendIdx)
			{
				Data::PParams BlendSection = BlendArray->Get<Data::PParams>(BlendIdx);
				if (!ProcessBlendSection(BlendSection, BlendIdx, Desc)) FAIL;
			}
		}
		else FAIL;
	}

	// Misc

	if (RS->Get(FlagValue, CStrID("AlphaTestEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Misc_AlphaTestEnable, FlagValue);

	if (RS->Get(IntValue, CStrID("AlphaTestRef"))) Desc.AlphaTestRef = IntValue;
	if (RS->Get(StrValue, CStrID("AlphaTestFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.AlphaTestFunc = StringToCmpFunc(StrValue);
	}

	Data::CData ClipPlanes;
	if (RS->Get(ClipPlanes, CStrID("ClipPlanes")))
	{
		if (ClipPlanes.IsA<bool>() && ClipPlanes == false)
		{
			// All clip planes disabled
		}
		else if (ClipPlanes.IsA<int>())
		{
			int CP = ClipPlanes;
			for (int i = 0; i < 5; ++i)
				Desc.Flags.SetTo(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << i, !!(CP & (1 << i)));
		}
		else if (ClipPlanes.IsA<Data::PDataArray>())
		{
			Data::CDataArray& CP = *ClipPlanes.GetValue<Data::PDataArray>();
			
			for (UPTR i = 0; i < 5; ++i)
				Desc.Flags.Clear(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << i);
			
			for (UPTR i = 0; i < CP.GetCount(); ++i)
			{
				Data::CData& Val = CP[i];
				if (!Val.IsA<int>()) continue;
				int IntVal = Val;
				if (IntVal < 0 || IntVal > 5) continue;
				Desc.Flags.Set(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << IntVal);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug)
{
	// Read effect source file

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, Buffer)) return ERR_IO_READ;

	Data::PParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Params)) return ERR_IO_READ;
	}

	Data::PParams Techs;
	Data::PParams RenderStates;
	if (!Params->Get(Techs, CStrID("Techniques"))) return ERR_INVALID_DATA;
	if (!Params->Get(RenderStates, CStrID("RenderStates"))) return ERR_INVALID_DATA;

	// Collect techs and render states they use

	CArray<CTechInfo> UsedTechs;
	CDict<CStrID, CRenderStateRef> UsedRenderStates;
	for (UPTR TechIdx = 0; TechIdx < Techs->GetCount(); ++TechIdx)
	{
		Data::CParam& Tech = Techs->Get(TechIdx);
		Data::PParams TechDesc = Tech.GetValue<Data::PParams>();

		Data::PDataArray Passes;
		if (!TechDesc->Get(Passes, CStrID("Passes"))) continue;
		CStrID InputSet = TechDesc->Get(CStrID("InputSet"), CStrID::Empty);
		if (!InputSet.IsValid()) continue;
		UPTR MaxLights = (UPTR)n_max(TechDesc->Get<int>(CStrID("MaxLights"), 0), 0);
		UPTR LightVariationCount = MaxLights + 1;

		CTechInfo* pTechInfo = UsedTechs.Add();
		pTechInfo->ID = Tech.GetName();
		pTechInfo->InputSet = InputSet;
		pTechInfo->MaxLights = MaxLights;

		for (UPTR PassIdx = 0; PassIdx < Passes->GetCount(); ++PassIdx)
		{
			CStrID PassID = Passes->Get<CStrID>(PassIdx);
			pTechInfo->Passes.Add(PassID);
			n_msg(VL_DETAILS, "Tech %s, Pass %d: %s, MaxLights: %d\n", Tech.GetName().CStr(), PassIdx, PassID.CStr(), MaxLights);
			IPTR Idx = UsedRenderStates.FindIndex(PassID);
			if (Idx == INVALID_INDEX)
			{
				CRenderStateRef& NewPass = UsedRenderStates.Add(PassID);
				NewPass.ID = PassID;
				NewPass.MaxLights = MaxLights;
				NewPass.ShaderIDs = (U32*)n_malloc(Render::ShaderType_COUNT * LightVariationCount * sizeof(U32));
			}
			else
			{
				CRenderStateRef& Pass = UsedRenderStates.ValueAt(Idx);
				if (MaxLights > Pass.MaxLights)
				{
					Pass.MaxLights = MaxLights;
					Pass.ShaderIDs = (U32*)n_realloc(Pass.ShaderIDs, Render::ShaderType_COUNT * LightVariationCount * sizeof(U32));
				}
			}
		}
	}

	// Compile and validate used render states, unwinding their hierarchy

	for (UPTR i = 0; i < UsedRenderStates.GetCount(); )
	{
		Data::PParams ShaderSections[Render::ShaderType_COUNT];

		// Load states only, collect shader sections
		CRenderStateRef& RSRef = UsedRenderStates.ValueAt(i);
		RSRef.Desc.SetDefaults();
		if (!ReadRenderStateDesc(RenderStates, RSRef.ID, RSRef.Desc, ShaderSections))
		{
			// Loading failed, discard this render state
			n_msg(VL_WARNING, "Render state '%s' parsing failed\n", UsedRenderStates.KeyAt(i).CStr());
			UsedRenderStates.RemoveAt(i);
			continue;
		}

		UPTR LightVariationCount = RSRef.MaxLights + 1;
		
		// Compile shaders from collected section for each light count variation
		for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
		{
			ProcessShaderSection(ShaderSections[ShaderType], (Render::EShaderType)ShaderType, Debug, RSRef);

			// If some of used shaders failed to load in all variations, discard this render state
			if (RSRef.UsesShader[ShaderType])
			{
				UPTR LightCount;
				for (LightCount = 0; LightCount < LightVariationCount; ++LightCount)
					if (RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] != 0) break;

				if (LightCount == LightVariationCount)
				{
					const char* pShaderNames[] = { "vertex", "pixel", "geometry", "hull", "domain" };
					n_msg(VL_WARNING, "Render state '%s' %s shader compilation failed for all variations\n", UsedRenderStates.KeyAt(i).CStr(), pShaderNames[ShaderType]);
					UsedRenderStates.RemoveAt(i);
					continue;
				}
			}
		}

		++i;
	}

	// Resolve pass refs in techs, discard light count variations and whole techs where at least one render state failed to compile
	// Reduce MaxLights if all variations above a value are invalid

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount();)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];

		UPTR LightVariationCount = TechInfo.MaxLights + 1;
		UPTR LastValidVariation = INVALID_INDEX;

		TechInfo.PassIndices.SetSize(TechInfo.Passes.GetCount());
		TechInfo.VariationValid.SetSize(LightVariationCount);

		bool DiscardTech = false;

		// Cache pass render state indices, discard tech if any pass was discarded
		for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
		{
			IPTR Idx = UsedRenderStates.FindIndex(TechInfo.Passes[PassIdx]);
			if (Idx == INVALID_INDEX)
			{
				DiscardTech = true;
				break;
			}
			else TechInfo.PassIndices[PassIdx] = (UPTR)Idx;
		}

		if (DiscardTech)
		{
			n_msg(VL_WARNING, "Tech '%s' discarded due to discarded passes\n", TechInfo.ID.CStr());
			UsedTechs.RemoveAt(TechIdx);
			continue;
		}

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			TechInfo.VariationValid[LightCount] = true;

			// If variation uses shader that failed to compile, discard variation
			for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
			{
				CRenderStateRef& RSRef = UsedRenderStates.ValueAt(TechInfo.PassIndices[PassIdx]);
				for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
				{
					if (RSRef.UsesShader[ShaderType] && RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] == 0)
					{
						TechInfo.VariationValid[LightCount] = false;
						break;
					}
				}
			}

			if (TechInfo.VariationValid[LightCount]) LastValidVariation = LightCount;
		}

		// Discard tech without valid variations
		if (LastValidVariation == INVALID_INDEX)
		{
			n_msg(VL_WARNING, "Tech '%s' discarded as it has no valid variations\n", TechInfo.ID.CStr());
			UsedTechs.RemoveAt(TechIdx);
			continue;
		}

		TechInfo.MaxLights = LastValidVariation;

		++TechIdx;
	}

	// Discard an effect if there are no valid techs

	if (!UsedTechs.GetCount())
	{
		n_msg(VL_WARNING, "Effect '%s' is not compiled, because it has no valid techs\n", pInFilePath);
		return ERR_INVALID_DATA;
	}

	// Build and validate material and tech constant maps

	//!!!NB: if the same param is used in different stages and in different CBs, setting it
	//in a tech requires passing CB instance per stage! May be restrict to use one param only
	//in the same CB in all stages or even use one param only in one stage instead.

	// Tech param is a param met in any pass, in any variation, in any shader stage
	// Inside one tech, a param for each pass must be the same or not defined
	// If one param in one tech is defined differently in different passes, FAIL
	// Each shader stage that param uses may define a param differently
	// Usage of one param more than in one shader stage should be generally avoided
	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];
		UPTR LightVariationCount = TechInfo.MaxLights + 1;

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
			{
				CRenderStateRef& RSRef = UsedRenderStates.ValueAt(TechInfo.PassIndices[PassIdx]);
				for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
				{
					if (RSRef.UsesShader[ShaderType])
					{
						U32 ShaderID = RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount];
						// If this shader is not processed yet (may appear more than once in a tech)
						// Load shader metadata
						// For each param of this shader
						// If not yet added, add new tech param
						// If first occurrence in this stage, store its metadata for a shader stage
						// Warn if there are more than one stage that parameter uses (warn once, when we add second stage to a param desc)
						// If already added, ensure metadata at this stage is the same
						// Also ensure CBs are the same for SM4.0+ (CB register and size)
					}
				}
			}
		}
	}
	// If we are here, it is guaranteed that all tech params are gathered and valid

	Data::PParams ParamsDesc;
	if (Params->Get(Techs, CStrID("GlobalParams")))
	{
		//
	}

	if (Params->Get(Techs, CStrID("MaterialParams")))
	{
		for (UPTR ParamIdx = 0; ParamIdx < ParamsDesc->GetCount(); ++ParamIdx)
		{
			Data::CParam& ShaderParam = ParamsDesc->Get(ParamIdx);
			for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
			{
				// Find current param in tech
				// If not found, OK, anyway material will set it
				// For each shader stage which uses this param
				// If [Name, Stage] first introduced, save its metadata and default value
				// Else compare its metadata against a stored one (including CB name, register & size for SM4.0+)
				// If different, fail, as materials can't be used in this case
			}
		}

		// After all, save material table
		// Name, DefaultValue, [stages?]
	}

	// Write result to a file

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath));
	
	IO::CFileStream File(pOutFilePath);
	if (!File.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	IO::CBinaryWriter W(File);

	if (!W.Write('SHFX')) return ERR_IO_WRITE;
	if (!W.Write<U32>(0x0100)) return ERR_IO_WRITE;

	if (!W.Write(UsedTechs.GetCount())) return ERR_IO_WRITE;

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];

		if (!W.Write(TechInfo.ID)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.InputSet)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.MaxLights)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.Passes.GetCount())) return ERR_IO_WRITE;

		// Save matrix of [render state ID + shader IDs] per-variation, per-pass
		// Save just one INVALID_INDEX for unsupported variations
		for (UPTR LightCount = 0; LightCount <= TechInfo.MaxLights; ++LightCount)
		{
			for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
			{
				//if (!W.Write(NewPass.ID)) return ERR_IO_WRITE;

				//???reference passes by UsedRenderStates index, not by name? slightly smaller file and faster init.
			}
		}
	}

	if (!W.Write(UsedRenderStates.GetCount())) return ERR_IO_WRITE;

	/*
	for (UPTR i = 0; i < UsedRenderStates.GetCount(); ++i)
	{
		CStrID ID = UsedRenderStates[i];
		Render::CToolRenderStateDesc Desc;
		Desc.SetDefaults();
		if (!ReadRenderStateDesc(RenderStates, ID, Desc, Debug, true)) return ERR_INVALID_DATA;

		//!!!if passes are referenced by index, don't save IDs!
		//???or by ID store in some global cross-effect renderstate database (res mgr)
		if (!W.Write(ID)) return ERR_IO_WRITE;

		if (!W.Write(Desc.VertexShader)) return ERR_IO_WRITE;
		if (!W.Write(Desc.PixelShader)) return ERR_IO_WRITE;
		if (!W.Write(Desc.GeometryShader)) return ERR_IO_WRITE;
		if (!W.Write(Desc.DomainShader)) return ERR_IO_WRITE;
		if (!W.Write(Desc.HullShader)) return ERR_IO_WRITE;

		if (!W.Write(Desc.Flags.GetMask())) return ERR_IO_WRITE;
		
		if (!W.Write(Desc.DepthBias)) return ERR_IO_WRITE;
		if (!W.Write(Desc.DepthBiasClamp)) return ERR_IO_WRITE;
		if (!W.Write(Desc.SlopeScaledDepthBias)) return ERR_IO_WRITE;
		
		if (Desc.Flags.Is(Render::CToolRenderStateDesc::DS_DepthEnable))
		{
			if (!W.Write((int)Desc.DepthFunc)) return ERR_IO_WRITE;
		}
	
		if (Desc.Flags.Is(Render::CToolRenderStateDesc::DS_StencilEnable))
		{
			if (!W.Write(Desc.StencilReadMask)) return ERR_IO_WRITE;
			if (!W.Write(Desc.StencilWriteMask)) return ERR_IO_WRITE;
			if (!W.Write(Desc.StencilRef)) return ERR_IO_WRITE;

			if (!W.Write((int)Desc.StencilFrontFace.StencilFailOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilFrontFace.StencilDepthFailOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilFrontFace.StencilPassOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilFrontFace.StencilFunc)) return ERR_IO_WRITE;

			if (!W.Write((int)Desc.StencilBackFace.StencilFailOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilBackFace.StencilDepthFailOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilBackFace.StencilPassOp)) return ERR_IO_WRITE;
			if (!W.Write((int)Desc.StencilBackFace.StencilFunc)) return ERR_IO_WRITE;
		}

		for (int BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && Desc.Flags.IsNot(Render::CToolRenderStateDesc::Blend_Independent)) break;
			if (Desc.Flags.IsNot(Render::CToolRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			Render::CToolRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];
			if (!W.Write((int)RTBlend.SrcBlendArg)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.DestBlendArg)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.BlendOp)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.SrcBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.DestBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.BlendOpAlpha)) return ERR_IO_WRITE;
			if (!W.Write((int)RTBlend.WriteMask)) return ERR_IO_WRITE;
		}

		if (!W.Write(Desc.BlendFactorRGBA[0])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[1])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[2])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[3])) return ERR_IO_WRITE;
		if (!W.Write(Desc.SampleMask)) return ERR_IO_WRITE;

		if (!W.Write(Desc.AlphaTestRef)) return ERR_IO_WRITE;
		if (!W.Write((int)Desc.AlphaTestFunc)) return ERR_IO_WRITE;
	};
	*/

	File.Close();

	//???Samplers? descs and per-tech register assignmemts? or per-whole-effect?
	// All shaders - register mappings
	// Unwind hierarchy here, store value-only whole RS descs
	// read ony referenced states, load bases on demand once and cache

	return SUCCESS;
}
//---------------------------------------------------------------------
