#include "ShaderReflectionSM30.h"

#include <Render/RenderFwd.h>	// For EGPUFeatureLevel
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <DEMD3DInclude.h>
#include <D3D9ShaderReflectionAPI.h>

extern CString Messages;

bool CSM30BufferMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	//const CSM30BufferMeta& TypedOther = (const CSM30BufferMeta&)Other;
	//return SlotIndex == TypedOther.SlotIndex;
	OK;
}
//---------------------------------------------------------------------

bool CSM30ConstMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CSM30ConstMeta& TypedOther = (const CSM30ConstMeta&)Other;
	return RegisterSet == TypedOther.RegisterSet &&
		RegisterStart == TypedOther.RegisterStart &&
		ElementRegisterCount == TypedOther.ElementRegisterCount &&
		ElementCount == TypedOther.ElementCount &&
		Flags == TypedOther.Flags;
}
//---------------------------------------------------------------------

bool CSM30RsrcMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CSM30RsrcMeta& TypedOther = (const CSM30RsrcMeta&)Other;
	return Register == TypedOther.Register;
}
//---------------------------------------------------------------------

bool CSM30SamplerMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) FAIL;
	const CSM30SamplerMeta& TypedOther = (const CSM30SamplerMeta&)Other;
	return Type == TypedOther.Type &&
		RegisterStart == TypedOther.RegisterStart &&
		RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

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

bool CSM30ShaderMeta::CollectFromBinaryAndSource(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CDEMD3DInclude& IncludeHandler)
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
	CSM30StructMeta* pStructMeta = Structs.Reserve(StructCount);
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
			CSM30SamplerMeta* pMeta = Samplers.Reserve(1);
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
						CSM30RsrcMeta* pMeta = Resources.Reserve(1);
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
			for (; BufferIndex < Buffers.GetCount(); ++BufferIndex)
				if (Buffers[BufferIndex].Name == BufferName) break;

			if (BufferIndex == Buffers.GetCount())
			{
				CSM30BufferMeta* pMeta = Buffers.Reserve(1);
				pMeta->Name = BufferName;
				pMeta->SlotIndex = SlotIndex;
			}
			else
			{
				CSM30BufferMeta& Meta = Buffers[BufferIndex];
				if (SlotIndex == (U32)(INVALID_INDEX))
					SlotIndex = SlotIndex;
				else if (SlotIndex != SlotIndex)
				{
					Messages += "CBuffer '";
					Messages += Meta.Name;
					Messages += "' is bound to different SlotIndex values (at least ";
					Messages += StringUtils::FromInt(SlotIndex);
					Messages += " and ";
					Messages += StringUtils::FromInt(SlotIndex);
					Messages += ") in the same shader, please fix it\n";
					FAIL;
				}
			}

			CSM30ConstMeta* pMeta = Consts.Reserve(1);
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

			CSM30BufferMeta& BufMeta = Buffers[pMeta->BufferIndex];
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
	while (i < Buffers.GetCount())
	{
		CSM30BufferMeta& B = Buffers[i];
		if (!B.UsedFloat4.GetCount() &&
			!B.UsedInt4.GetCount() &&
			!B.UsedBool.GetCount())
		{
			Buffers.RemoveAt(i);
		}
		else
		{
			UsedSlotIndices.Add(B.SlotIndex);
			++i;
		}
	};

	UsedSlotIndices.Sort();

	U32 NewSlotIndex = 0;
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CSM30BufferMeta& B = Buffers[i];
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

bool CSM30ShaderMeta::Save(IO::CBinaryWriter& W) const
{
	W.Write<U32>(Buffers.GetCount());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CSM30BufferMeta& Obj = Buffers[i];
		Obj.UsedFloat4.Sort();
		Obj.UsedInt4.Sort();
		Obj.UsedBool.Sort();

		W.Write(Obj.Name);
		W.Write(Obj.SlotIndex);

		WriteRegisterRanges(Obj.UsedFloat4, W, "float4");
		WriteRegisterRanges(Obj.UsedInt4, W, "int4");
		WriteRegisterRanges(Obj.UsedBool, W, "bool");
	}

	W.Write<U32>(Structs.GetCount());
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CSM30StructMeta& Obj = Structs[i];

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

	W.Write<U32>(Consts.GetCount());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const CSM30ConstMeta& Obj = Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<U8>(Obj.RegisterSet);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.ElementRegisterCount);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Flags);
	}

	W.Write<U32>(Resources.GetCount());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const CSM30RsrcMeta& Obj = Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
	}

	W.Write<U32>(Samplers.GetCount());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const CSM30SamplerMeta& Obj = Samplers[i];
		W.Write(Obj.Name);
		W.Write<U8>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	OK;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Load(IO::CBinaryReader& R)
{
	Buffers.Clear();
	Consts.Clear();
	Samplers.Clear();

	U32 Count;

	R.Read<U32>(Count);
	CSM30BufferMeta* pBuf = Buffers.Reserve(Count, false);
	for (; pBuf < Buffers.End(); ++pBuf)
	{
		CSM30BufferMeta& Obj = *pBuf;

		R.Read(Obj.Name);
		R.Read(Obj.SlotIndex);

		ReadRegisterRanges(Obj.UsedFloat4, R);
		ReadRegisterRanges(Obj.UsedInt4, R);
		ReadRegisterRanges(Obj.UsedBool, R);
	}

	R.Read<U32>(Count);
	CSM30StructMeta* pStruct = Structs.Reserve(Count, false);
	for (; pStruct < Structs.End(); ++pStruct)
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
	CSM30ConstMeta* pConst = Consts.Reserve(Count, false);
	for (; pConst < Consts.End(); ++pConst)
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
	CSM30RsrcMeta* pRsrc = Resources.Reserve(Count, false);
	for (; pRsrc < Resources.End(); ++pRsrc)
	{
		CSM30RsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
	}

	R.Read<U32>(Count);
	CSM30SamplerMeta* pSamp = Samplers.Reserve(Count, false);
	for (; pSamp < Samplers.End(); ++pSamp)
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

U32 CSM30ShaderMeta::GetMinFeatureLevel() const
{
	return Render::GPU_Level_D3D9_3;
}
//---------------------------------------------------------------------

UPTR CSM30ShaderMeta::GetParamCount(EShaderParamClass Class) const
{
	switch (Class)
	{
		case ShaderParam_Const:		return Consts.GetCount();
		case ShaderParam_Resource:	return Resources.GetCount();
		case ShaderParam_Sampler:	return Samplers.GetCount();
		default:					return 0;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CSM30ShaderMeta::GetParamObject(EShaderParamClass Class, UPTR Index)
{
	switch (Class)
	{
		case ShaderParam_Const:		return &Consts[Index];
		case ShaderParam_Resource:	return &Resources[Index];
		case ShaderParam_Sampler:	return &Samplers[Index];
		default:					return NULL;
	}
}
//---------------------------------------------------------------------

UPTR CSM30ShaderMeta::AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetShaderModel() != GetShaderModel() || pMetaObject->GetClass() != Class) return (UPTR)(INVALID_INDEX);

	switch (Class)
	{
		case ShaderParam_Const:
		{
			Consts.Add(*(const CSM30ConstMeta*)pMetaObject);
			return Consts.GetCount() - 1;
		}
		case ShaderParam_Resource:
		{
			Resources.Add(*(const CSM30RsrcMeta*)pMetaObject);
			return Resources.GetCount() - 1;
		}
		case ShaderParam_Sampler:
		{
			Samplers.Add(*(const CSM30SamplerMeta*)pMetaObject);
			return Samplers.GetCount() - 1;
		}
		default:	return (UPTR)(INVALID_INDEX);
	}
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::FindParamObjectByName(EShaderParamClass Class, const char* pName, UPTR& OutIndex) const
{
	switch (Class)
	{
		case ShaderParam_Const:
		{
			UPTR Idx = 0;
			for (; Idx < Consts.GetCount(); ++ Idx)
				if (Consts[Idx].Name == pName) break;
			if (Idx == Consts.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		case ShaderParam_Resource:
		{
			UPTR Idx = 0;
			for (; Idx < Resources.GetCount(); ++ Idx)
				if (Resources[Idx].Name == pName) break;
			if (Idx == Resources.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		case ShaderParam_Sampler:
		{
			UPTR Idx = 0;
			for (; Idx < Samplers.GetCount(); ++ Idx)
				if (Samplers[Idx].Name == pName) break;
			if (Idx == Samplers.GetCount()) FAIL;
			OutIndex = Idx;
			OK;
		}
		default:					FAIL;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CSM30ShaderMeta::GetContainingConstantBuffer(CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetClass() != ShaderParam_Const || pMetaObject->GetShaderModel() != GetShaderModel()) return NULL;
	return &Buffers[((CSM30ConstMeta*)pMetaObject)->BufferIndex];
}
//---------------------------------------------------------------------
