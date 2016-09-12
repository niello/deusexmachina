#include "ShaderReflection.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <Render/RenderFwd.h>		// For GPU levels enum
#include <DEMD3DInclude.h>
#include <D3D9ShaderReflection.h>
#include <D3DCompiler.inl>

extern CString Messages;

static void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
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
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
	}

	U64 EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<U32>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

static void ReadRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryReader& R)
{
	UsedRegs.Clear();

	U32 RangeCount = 0;
	R.Read<U32>(RangeCount);
	for (UPTR i = 0; i < RangeCount; ++i)
	{
		U32 Curr;
		U32 CurrCount;
		R.Read<U32>(Curr);
		R.Read<U32>(CurrCount);
		U32 CurrEnd = Curr + CurrCount;
		for (; Curr < CurrEnd; ++Curr)
			UsedRegs.Add((UPTR)Curr);
	}
}
//---------------------------------------------------------------------

bool SM30CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CDEMD3DInclude& IncludeHandler, CSM30ShaderMeta& Out)
{
	CArray<CD3D9ConstantDesc> D3D9Consts;
	CDict<U32, CD3D9StructDesc> D3D9Structs;
	CString Creator;

	if (!D3D9Reflect(pData, Size, D3D9Consts, D3D9Structs, Creator)) FAIL;

	// Process source code includes and remove comments

	CString Source(pSource, SourceSize);

	//!!!DIRTY HACK! Need valid way to edit CString inplace by StripComments()!
	UPTR NewLength = StringUtils::StripComments(&Source[0]);
	Source.TruncateRight(Source.GetLength() - NewLength);

	IPTR CurrIdx = 0;
	IPTR IncludeIdx;
	while ((IncludeIdx = Source.FindIndex("#include", CurrIdx)) != INVALID_INDEX)
	{
		IPTR FileNameStartSys = Source.FindIndex('<', IncludeIdx);
		IPTR FileNameStartLocal = Source.FindIndex('\"', IncludeIdx);
		if (FileNameStartSys == INVALID_INDEX && FileNameStartLocal == INVALID_INDEX)
		{
			Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
			break;
		}

		if (FileNameStartSys != INVALID_INDEX && FileNameStartLocal != INVALID_INDEX)
		{
			if (FileNameStartSys < FileNameStartLocal) FileNameStartLocal = INVALID_INDEX;
			else FileNameStartSys = INVALID_INDEX;
		}

		D3D_INCLUDE_TYPE IncludeType;
		IPTR FileNameStart;
		IPTR FileNameEnd;
		if (FileNameStartSys != INVALID_INDEX)
		{
			IncludeType = D3D_INCLUDE_SYSTEM; // <FileName>
			FileNameStart = FileNameStartSys + 1;
			FileNameEnd = Source.FindIndex('>', FileNameStart);
		}
		else
		{
			IncludeType = D3D_INCLUDE_LOCAL; // "FileName"
			FileNameStart = FileNameStartLocal + 1;
			FileNameEnd = Source.FindIndex('\"', FileNameStart);
		}

		if (FileNameEnd == INVALID_INDEX)
		{
			Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
			break;
		}

		CString IncludeFileName = Source.SubString(FileNameStart, FileNameEnd - FileNameStart);

		const void* pData;
		UINT Bytes;
		if (FAILED(IncludeHandler.Open(IncludeType, IncludeFileName.CStr(), NULL, &pData, &Bytes)))
		{
			Messages += "SM30CollectShaderMetadata() > Failed to #include '";
			Messages += IncludeFileName;
			Messages += "', skipped\n";
			CurrIdx = FileNameEnd;
			continue;
		}

		Source =
			Source.SubString(0, IncludeIdx) +
			CString((const char*)pData, Bytes) +
			Source.SubString(FileNameEnd + 1, Source.GetLength() - FileNameEnd - 1);
		
		IncludeHandler.Close(pData);

		CurrIdx = IncludeIdx;

		//!!!DIRTY HACK! Need valid way to edit CString inplace by StripComments()!
		NewLength = StringUtils::StripComments(&Source[0]);
		Source.TruncateRight(Source.GetLength() - NewLength);
	}

	// Collect structure layout metadata

	const UPTR StructCount = D3D9Structs.GetCount();
	CSM30StructMeta* pStructMeta = Out.Structs.Reserve(StructCount);
	for (UPTR i = 0; i < StructCount; ++i, ++pStructMeta)
	{
		const CD3D9StructDesc& D3D9StructDesc = D3D9Structs.ValueAt(i);

		const UPTR MemberCount = D3D9StructDesc.Members.GetCount();
		CSM30StructMemberMeta* pMemberMeta = pStructMeta->Members.Reserve(MemberCount);
		for (UPTR j = 0; j < MemberCount; ++j, ++pMemberMeta)
		{
			const CD3D9ConstantDesc& D3D9ConstDesc = D3D9StructDesc.Members[j];
			pMemberMeta->Name = D3D9ConstDesc.Name;
			pMemberMeta->StructIndex = (U32)D3D9Structs.FindIndex(D3D9ConstDesc.StructID);
			pMemberMeta->RegisterOffset = D3D9ConstDesc.RegisterIndex;
			pMemberMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMemberMeta->ElementCount = D3D9ConstDesc.Type.Elements;
			pMemberMeta->Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				pMemberMeta->Flags |= ShaderConst_ColumnMajor;

			n_assert(pMemberMeta->ElementRegisterCount * pMemberMeta->ElementCount == D3D9ConstDesc.RegisterCount);
		}
	}

	// Collect constant metadata

	CDict<CString, CArray<CString>> SampToTex;
	D3D9FindSamplerTextures(Source.CStr(), SampToTex);

	CDict<CString, CString> ConstToBuf;

	for (UPTR i = 0; i < D3D9Consts.GetCount(); ++i)
	{
		CD3D9ConstantDesc& D3D9ConstDesc = D3D9Consts[i];

		if (D3D9ConstDesc.RegisterSet == RS_MIXED)
		{
			Messages += "    SM3.0 mixed-regset structs aren't supported, '";
			Messages += D3D9ConstDesc.Name;
			Messages += "' skipped\n";
			continue;
		}
		else if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CSM30SamplerMeta* pMeta = Out.Samplers.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->RegisterCount = D3D9ConstDesc.RegisterCount;

			switch (D3D9ConstDesc.Type.Type)
			{
				case PT_SAMPLER1D:		pMeta->Type = SM30Sampler_1D; break;
				case PT_SAMPLER3D:		pMeta->Type = SM30Sampler_3D; break;
				case PT_SAMPLERCUBE:	pMeta->Type = SM30Sampler_CUBE; break;
				default:				pMeta->Type = SM30Sampler_2D; break;
			}

			UPTR TexCount;
			int STIdx = SampToTex.FindIndex(D3D9ConstDesc.Name);
			if (STIdx == INVALID_INDEX) TexCount = 0;
			else
			{
				const CArray<CString>& TexNames = SampToTex.ValueAt(STIdx);
				TexCount = n_min(D3D9ConstDesc.RegisterCount, TexNames.GetCount());
				for (UPTR TexIdx = 0; TexIdx < TexCount; ++TexIdx)
				{
					const CString& TexName = TexNames[TexIdx];
					if (TexName.IsValid())
					{
						CSM30RsrcMeta* pMeta = Out.Resources.Reserve(1);
						pMeta->Name = TexName;
						pMeta->Register = D3D9ConstDesc.RegisterIndex + TexIdx;
					}
					else if (D3D9ConstDesc.RegisterCount > 1)
					{
						Messages += "Sampler '";
						Messages += D3D9ConstDesc.Name;
						Messages += '[';
						Messages += StringUtils::FromInt(TexIdx);
						Messages += "]' has no texture bound, use initializer in a form of 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
					}
					else
					{
						Messages += "Sampler '";
						Messages += D3D9ConstDesc.Name;
						Messages += "' has no texture bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'\n";
					}
				}
			}

			if (!TexCount)
			{
				Messages += "Sampler '";
				Messages += D3D9ConstDesc.Name;
				Messages += "' has no textures bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }' or 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
			}
		}
		else // Constants
		{
			CString BufferName;
			U32 SlotIndex = (U32)(INVALID_INDEX);
			D3D9FindConstantBuffer(Source.CStr(), D3D9ConstDesc.Name, BufferName, SlotIndex);

			UPTR BufferIndex = 0;
			for (; BufferIndex < Out.Buffers.GetCount(); ++BufferIndex)
				if (Out.Buffers[BufferIndex].Name == BufferName) break;

			if (BufferIndex == Out.Buffers.GetCount())
			{
				CSM30BufferMeta* pMeta = Out.Buffers.Reserve(1);
				pMeta->Name = BufferName;
				pMeta->SlotIndex = SlotIndex;
			}
			else
			{
				CSM30BufferMeta& Meta = Out.Buffers[BufferIndex];
				if (Meta.SlotIndex == (U32)(INVALID_INDEX))
					Meta.SlotIndex = SlotIndex;
				else if (Meta.SlotIndex != SlotIndex)
				{
					Messages += "CBuffer '";
					Messages += Meta.Name;
					Messages += "' is bound to different SlotIndex values (at least ";
					Messages += StringUtils::FromInt(Meta.SlotIndex);
					Messages += " and ";
					Messages += StringUtils::FromInt(SlotIndex);
					Messages += ") in the same shader, please fix it\n";
					FAIL;
				}
			}

			CSM30ConstMeta* pMeta = Out.Consts.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->BufferIndex = BufferIndex;
			pMeta->StructIndex = (U32)D3D9Structs.FindIndex(D3D9ConstDesc.StructID);

			switch (D3D9ConstDesc.RegisterSet)
			{
				case RS_FLOAT4:	pMeta->RegisterSet = RS_Float4; break;
				case RS_INT4:	pMeta->RegisterSet = RS_Int4; break;
				case RS_BOOL:	pMeta->RegisterSet = RS_Bool; break;
				default:
				{
					Sys::Error("Unsupported SM3.0 register set %d\n", D3D9ConstDesc.RegisterSet);
					FAIL;
				}
			};

			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMeta->ElementCount = D3D9ConstDesc.Type.Elements;
			pMeta->Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				pMeta->Flags |= ShaderConst_ColumnMajor;

			// Cache value
			pMeta->RegisterCount = pMeta->ElementRegisterCount * pMeta->ElementCount;

			n_assert(pMeta->RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30BufferMeta& BufMeta = Out.Buffers[pMeta->BufferIndex];
			CArray<UPTR>& UsedRegs = (pMeta->RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 : ((pMeta->RegisterSet == RS_Int4) ? BufMeta.UsedInt4 : BufMeta.UsedBool);
			for (UPTR r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				if (!UsedRegs.Contains(r)) UsedRegs.Add(r);
			}
		}
	}

	// Remove empty constant buffers and set free SlotIndices where no explicit value was specified

	CArray<U32> UsedSlotIndices;
	UPTR i = 0;
	while (i < Out.Buffers.GetCount())
	{
		CSM30BufferMeta& B = Out.Buffers[i];
		if (!B.UsedFloat4.GetCount() &&
			!B.UsedInt4.GetCount() &&
			!B.UsedBool.GetCount())
		{
			Out.Buffers.RemoveAt(i);
		}
		else
		{
			UsedSlotIndices.Add(B.SlotIndex);
			++i;
		}
	};

	UsedSlotIndices.Sort();

	U32 NewSlotIndex = 0;
	for (UPTR i = 0; i < Out.Buffers.GetCount(); ++i)
	{
		CSM30BufferMeta& B = Out.Buffers[i];
		if (B.SlotIndex == (U32)(INVALID_INDEX))
		{
			while (UsedSlotIndices.ContainsSorted(NewSlotIndex))
				++NewSlotIndex;
			B.SlotIndex = NewSlotIndex;
			++NewSlotIndex;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool SM30SaveShaderMetadata(IO::CBinaryWriter& W, const CSM30ShaderMeta& Meta)
{
	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		CSM30BufferMeta& Obj = Meta.Buffers[i];
		Obj.UsedFloat4.Sort();
		Obj.UsedInt4.Sort();
		Obj.UsedBool.Sort();

		W.Write(Obj.Name);
		W.Write(Obj.SlotIndex);

		WriteRegisterRanges(Obj.UsedFloat4, W, "float4");
		WriteRegisterRanges(Obj.UsedInt4, W, "int4");
		WriteRegisterRanges(Obj.UsedBool, W, "bool");
	}

	W.Write<U32>(Meta.Structs.GetCount());
	for (UPTR i = 0; i < Meta.Structs.GetCount(); ++i)
	{
		CSM30StructMeta& Obj = Meta.Structs[i];

		W.Write<U32>(Obj.Members.GetCount());
		for (UPTR j = 0; j < Obj.Members.GetCount(); ++j)
		{
			CSM30StructMemberMeta& Member = Obj.Members[j];
			W.Write(Member.Name);
			W.Write(Member.StructIndex);
			W.Write(Member.RegisterOffset);
			W.Write(Member.ElementRegisterCount);
			W.Write(Member.ElementCount);
			W.Write(Member.Flags);
		}
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CSM30ConstMeta& Obj = Meta.Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<U8>(Obj.RegisterSet);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.ElementRegisterCount);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Flags);
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		const CSM30RsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CSM30SamplerMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool SM30LoadShaderMetadata(IO::CBinaryReader& R, CSM30ShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Samplers.Clear();

	U32 Count;

	R.Read<U32>(Count);
	CSM30BufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		CSM30BufferMeta& Obj = *pBuf;

		R.Read(Obj.Name);
		R.Read(Obj.SlotIndex);

		ReadRegisterRanges(Obj.UsedFloat4, R);
		ReadRegisterRanges(Obj.UsedInt4, R);
		ReadRegisterRanges(Obj.UsedBool, R);
	}

	R.Read<U32>(Count);
	CSM30StructMeta* pStruct = Meta.Structs.Reserve(Count, false);
	for (; pStruct < Meta.Structs.End(); ++pStruct)
	{
		CSM30StructMeta& Obj = *pStruct;

		R.Read<U32>(Count);
		CSM30StructMemberMeta* pMember = Obj.Members.Reserve(Count, false);
		for (; pMember < Obj.Members.End(); ++pMember)
		{
			CSM30StructMemberMeta& Member = *pMember;
			R.Read(Member.Name);
			R.Read(Member.StructIndex);
			R.Read(Member.RegisterOffset);
			R.Read(Member.ElementRegisterCount);
			R.Read(Member.ElementCount);
			R.Read(Member.Flags);
		}
	}

	R.Read<U32>(Count);
	CSM30ConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CSM30ConstMeta& Obj = *pConst;

		U8 RegSet;

		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.StructIndex);
		R.Read<U8>(RegSet);

		Obj.RegisterSet = (ESM30RegisterSet)RegSet;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.ElementRegisterCount);
		R.Read(Obj.ElementCount);
		R.Read(Obj.Flags);

		// Cache value
		Obj.RegisterCount = Obj.ElementRegisterCount * Obj.ElementCount;
	}

	R.Read<U32>(Count);
	CSM30RsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		CSM30RsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
	}

	R.Read<U32>(Count);
	CSM30SamplerMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CSM30SamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		
		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (ESM30SamplerType)Type;
		
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

// StructCache stores D3D11 type to metadata index mapping, where metadata index is an index in the Out.Structs array
static bool USMProcessStructure(ID3D11ShaderReflectionType* pType, U32 StructSize, CUSMShaderMeta& Out, CDict<ID3D11ShaderReflectionType*, UPTR>& StructCache)
{
	if (!pType) FAIL;

	// Already processed
	if (StructCache.FindIndex(pType) != INVALID_INDEX) OK;

	D3D11_SHADER_TYPE_DESC D3DTypeDesc;
	pType->GetDesc(&D3DTypeDesc);

	// Has no members
	if (D3DTypeDesc.Members == 0) OK;

	// Add and fill new structure layout metadata
	StructCache.Add(pType, Out.Structs.GetCount());
	CUSMStructMeta& Meta = *Out.Structs.Add();

	CUSMStructMemberMeta* pMemberMeta = Meta.Members.Reserve(D3DTypeDesc.Members);
	for (UINT MemberIdx = 0; MemberIdx < D3DTypeDesc.Members; ++MemberIdx, ++pMemberMeta)
	{
		LPCSTR pMemberName = pType->GetMemberTypeName(MemberIdx);

		ID3D11ShaderReflectionType* pMemberType = pType->GetMemberTypeByIndex(MemberIdx);
		D3D11_SHADER_TYPE_DESC D3DMemberTypeDesc;
		pMemberType->GetDesc(&D3DMemberTypeDesc);

		pMemberMeta->Name = pMemberName;
		pMemberMeta->Offset = D3DMemberTypeDesc.Offset;
		pMemberMeta->Flags = 0;

		U32 MemberSize;
		if (MemberIdx + 1 < D3DTypeDesc.Members)
		{
			ID3D11ShaderReflectionType* pNextMemberType = pType->GetMemberTypeByIndex(MemberIdx + 1);
			D3D11_SHADER_TYPE_DESC D3DNextMemberTypeDesc;
			pNextMemberType->GetDesc(&D3DNextMemberTypeDesc);
			MemberSize = D3DNextMemberTypeDesc.Offset - D3DMemberTypeDesc.Offset;
		}
		else MemberSize = StructSize - D3DMemberTypeDesc.Offset;

		if (D3DMemberTypeDesc.Elements > 1)
		{
			// Arrays
			pMemberMeta->ElementSize = MemberSize / D3DMemberTypeDesc.Elements;
			pMemberMeta->ElementCount = D3DMemberTypeDesc.Elements;
		}
		else
		{
			// Non-arrays and arrays [1]
			pMemberMeta->ElementSize = MemberSize;
			pMemberMeta->ElementCount = 1;
		}

		if (D3DMemberTypeDesc.Class == D3D_SVC_STRUCT)
		{
			pMemberMeta->Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
			if (!USMProcessStructure(pMemberType, pMemberMeta->ElementSize, Out, StructCache)) continue;
			pMemberMeta->StructIndex = StructCache[pMemberType];
		}
		else
		{
			pMemberMeta->StructIndex = (U32)(-1);
			switch (D3DMemberTypeDesc.Type)
			{
				case D3D_SVT_BOOL:	pMemberMeta->Type = USMConst_Bool; break;
				case D3D_SVT_INT:
				case D3D_SVT_UINT:	pMemberMeta->Type = USMConst_Int; break;
				case D3D_SVT_FLOAT:	pMemberMeta->Type = USMConst_Float; break;
				default:
				{
					Messages += "Unsupported constant '";
					Messages += pMemberName;
					Messages += "' type '";
					Messages += StringUtils::FromInt(D3DMemberTypeDesc.Type);
					Messages += "' in a structure '";
					Messages += (D3DTypeDesc.Name ? D3DTypeDesc.Name : "");
					Messages += "'\n";
					FAIL;
				}
			}

			if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
				pMemberMeta->Flags |= ShaderConst_ColumnMajor;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool USMCollectShaderMetadata(const void* pData, UPTR Size, CUSMShaderMeta& Out)
{
	ID3D11ShaderReflection* pReflector = NULL;

	if (FAILED(D3D11Reflect(pData, Size, &pReflector))) FAIL;

	D3D_FEATURE_LEVEL D3DFeatureLevel;
	if (FAILED(pReflector->GetMinFeatureLevel(&D3DFeatureLevel)))
	{
		pReflector->Release();
		FAIL;
	}

	switch (D3DFeatureLevel)
	{
		case D3D_FEATURE_LEVEL_9_1:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_1; break;
		case D3D_FEATURE_LEVEL_9_2:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_2; break;
		case D3D_FEATURE_LEVEL_9_3:		Out.MinFeatureLevel = Render::GPU_Level_D3D9_3; break;
		case D3D_FEATURE_LEVEL_10_0:	Out.MinFeatureLevel = Render::GPU_Level_D3D10_0; break;
		case D3D_FEATURE_LEVEL_10_1:	Out.MinFeatureLevel = Render::GPU_Level_D3D10_1; break;
		case D3D_FEATURE_LEVEL_11_0:	Out.MinFeatureLevel = Render::GPU_Level_D3D11_0; break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:						Out.MinFeatureLevel = Render::GPU_Level_D3D11_1; break;
	}

	Out.RequiresFlags = pReflector->GetRequiresFlags();

	D3D11_SHADER_DESC D3DDesc;
	if (FAILED(pReflector->GetDesc(&D3DDesc)))
	{
		pReflector->Release();
		FAIL;
	}

	CDict<ID3D11ShaderReflectionType*, UPTR> StructCache;

	for (UINT RsrcIdx = 0; RsrcIdx < D3DDesc.BoundResources; ++RsrcIdx)
	{
		D3D11_SHADER_INPUT_BIND_DESC RsrcDesc;
		if (FAILED(pReflector->GetResourceBindingDesc(RsrcIdx, &RsrcDesc)))
		{
			pReflector->Release();
			FAIL;
		}

		// D3D_SIF_USERPACKED - may fail assertion if not set!

		switch (RsrcDesc.Type)
		{
			case D3D_SIT_TEXTURE:
			{
				CUSMRsrcMeta* pMeta = Out.Resources.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->RegisterStart = RsrcDesc.BindPoint;
				pMeta->RegisterCount = RsrcDesc.BindCount;

				switch (RsrcDesc.Dimension)
				{
					case D3D_SRV_DIMENSION_TEXTURE1D:			pMeta->Type = USMRsrc_Texture1D; break;
					case D3D_SRV_DIMENSION_TEXTURE1DARRAY:		pMeta->Type = USMRsrc_Texture1DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2D:			pMeta->Type = USMRsrc_Texture2D; break;
					case D3D_SRV_DIMENSION_TEXTURE2DARRAY:		pMeta->Type = USMRsrc_Texture2DArray; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMS:			pMeta->Type = USMRsrc_Texture2DMS; break;
					case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:	pMeta->Type = USMRsrc_Texture2DMSArray; break;
					case D3D_SRV_DIMENSION_TEXTURE3D:			pMeta->Type = USMRsrc_Texture3D; break;
					case D3D_SRV_DIMENSION_TEXTURECUBE:			pMeta->Type = USMRsrc_TextureCUBE; break;
					case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:	pMeta->Type = USMRsrc_TextureCUBEArray; break;
					default:									pMeta->Type = USMRsrc_Unknown; break;
				}

				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				CUSMSamplerMeta* pMeta = Out.Samplers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->RegisterStart = RsrcDesc.BindPoint;
				pMeta->RegisterCount = RsrcDesc.BindCount;
				break;
			}
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
			case D3D_SIT_STRUCTURED:
			{
				// Can't violate this condition (can't define cbuffer / tbuffer array)
				n_assert(RsrcDesc.BindCount == 1);

				// There can be cbuffer and tbuffer with the same name, so search by name and type
				D3D_CBUFFER_TYPE DesiredType;
				switch (RsrcDesc.Type)
				{
					case D3D_SIT_CBUFFER:		DesiredType = D3D_CT_CBUFFER; break;
					case D3D_SIT_TBUFFER:		DesiredType = D3D_CT_TBUFFER; break;
					case D3D_SIT_STRUCTURED:	DesiredType = D3D_CT_RESOURCE_BIND_INFO; break;
				}

				ID3D11ShaderReflectionConstantBuffer* pCB = NULL;
				D3D11_SHADER_BUFFER_DESC D3DBufDesc;
				for (UPTR BufIdx = 0; BufIdx < D3DDesc.ConstantBuffers; ++BufIdx)
				{
					ID3D11ShaderReflectionConstantBuffer* pCurrCB = pReflector->GetConstantBufferByIndex(BufIdx);
					pCurrCB->GetDesc(&D3DBufDesc);
					if (!strcmp(RsrcDesc.Name, D3DBufDesc.Name) && D3DBufDesc.Type == DesiredType)
					{
						pCB = pCurrCB;
						break;
					}
				}
				
				if (!pCB || !D3DBufDesc.Variables) continue;

				//D3DBufDesc.uFlags & D3D_CBF_USERPACKED

				DWORD TypeMask;
				if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = USMBuffer_Texture;
				else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = USMBuffer_Structured;
				else TypeMask = 0;

				CUSMBufferMeta* pMeta = Out.Buffers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
				pMeta->Size = D3DBufDesc.Size;

				if (RsrcDesc.Type != D3D_SIT_STRUCTURED)
				{
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

						CUSMConstMeta* pConstMeta = Out.Consts.Reserve(1);
						pConstMeta->Name = D3DVarDesc.Name;
						pConstMeta->BufferIndex = Out.Buffers.GetCount() - 1;
						pConstMeta->Offset = D3DVarDesc.StartOffset;
						pConstMeta->Flags = 0;

						if (D3DTypeDesc.Elements > 1)
						{
							// Arrays
							pConstMeta->ElementSize = D3DVarDesc.Size / D3DTypeDesc.Elements;
							pConstMeta->ElementCount = D3DTypeDesc.Elements;
						}
						else
						{
							// Non-arrays and arrays [1]
							pConstMeta->ElementSize = D3DVarDesc.Size;
							pConstMeta->ElementCount = 1;
						}

						if (D3DTypeDesc.Class == D3D_SVC_STRUCT)
						{
							pConstMeta->Type = USMConst_Struct; // D3DTypeDesc.Type is 'void'
							if (!USMProcessStructure(pVarType, pConstMeta->ElementSize, Out, StructCache)) continue;
							pConstMeta->StructIndex = StructCache[pVarType];
						}
						else
						{
							pConstMeta->StructIndex = (U32)(-1);
							switch (D3DTypeDesc.Type)
							{
								case D3D_SVT_BOOL:	pConstMeta->Type = USMConst_Bool; break;
								case D3D_SVT_INT:
								case D3D_SVT_UINT:	pConstMeta->Type = USMConst_Int; break;
								case D3D_SVT_FLOAT:	pConstMeta->Type = USMConst_Float; break;
								default:
								{
									Messages += "Unsupported constant '";
									Messages += D3DVarDesc.Name;
									Messages += "' type '";
									Messages += StringUtils::FromInt(D3DTypeDesc.Type);
									Messages += "' in USM buffer '";
									Messages += RsrcDesc.Name;
									Messages += "'\n";
									FAIL;
								}
							}

							if (D3DTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
								pConstMeta->Flags |= ShaderConst_ColumnMajor;
						}
					}
				}

				break;
			}
		}
	}

	pReflector->Release();

	OK;
}
//---------------------------------------------------------------------

bool USMSaveShaderMetadata(IO::CBinaryWriter& W, const CUSMShaderMeta& Meta)
{
	W.Write<U32>(Meta.MinFeatureLevel);
	W.Write<U64>(Meta.RequiresFlags);

	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		const CUSMBufferMeta& Obj = Meta.Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.Size);
		//W.Write(Obj.ElementCount);
	}

	W.Write<U32>(Meta.Structs.GetCount());
	for (UPTR i = 0; i < Meta.Structs.GetCount(); ++i)
	{
		CUSMStructMeta& Obj = Meta.Structs[i];

		W.Write<U32>(Obj.Members.GetCount());
		for (UPTR j = 0; j < Obj.Members.GetCount(); ++j)
		{
			CUSMStructMemberMeta& Member = Obj.Members[j];
			W.Write(Member.Name);
			W.Write(Member.StructIndex);
			W.Write<U8>(Member.Type);
			W.Write(Member.Offset);
			W.Write(Member.ElementSize);
			W.Write(Member.ElementCount);
			W.Write(Member.Flags);
		}
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CUSMConstMeta& Obj = Meta.Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.Offset);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Flags);
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		const CUSMRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CUSMSamplerMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool USMLoadShaderMetadata(IO::CBinaryReader& R, CUSMShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Resources.Clear();
	Meta.Samplers.Clear();

	R.Read<U32>(Meta.MinFeatureLevel);
	R.Read<U64>(Meta.RequiresFlags);

	U32 Count;

	R.Read<U32>(Count);
	CUSMBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CUSMBufferMeta& Obj = *pBuf;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		R.Read(Obj.Size);
		///R.Read(Obj.ElementCount);
	}

	R.Read<U32>(Count);
	CUSMStructMeta* pStruct = Meta.Structs.Reserve(Count, false);
	for (; pStruct < Meta.Structs.End(); ++pStruct)
	{
		CUSMStructMeta& Obj = *pStruct;

		R.Read<U32>(Count);
		CUSMStructMemberMeta* pMember = Obj.Members.Reserve(Count, false);
		for (; pMember < Obj.Members.End(); ++pMember)
		{
			CUSMStructMemberMeta& Member = *pMember;
			R.Read(Member.Name);
			R.Read(Member.StructIndex);

			U8 Type;
			R.Read<U8>(Type);
			Member.Type = (EUSMConstType)Type;

			R.Read(Member.Offset);
			R.Read(Member.ElementSize);
			R.Read(Member.ElementCount);
			R.Read(Member.Flags);
		}
	}

	R.Read<U32>(Count);
	CUSMConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CUSMConstMeta& Obj = *pConst;
		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.StructIndex);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (EUSMConstType)Type;

		R.Read(Obj.Offset);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
		R.Read(Obj.Flags);
	}

	R.Read<U32>(Count);
	CUSMRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		CUSMRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);

		U8 Type;
		R.Read<U8>(Type);
		Obj.Type = (EUSMResourceType)Type;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	R.Read<U32>(Count);
	CUSMSamplerMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CUSMSamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------
