#include "ShaderReflectionSM30.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/StringUtils.h>
#include <DEMD3DInclude.h>
#include <D3D9ShaderReflectionAPI.h>

extern std::string Messages;

bool CSM30BufferMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	//const CSM30BufferMeta& TypedOther = (const CSM30BufferMeta&)Other;
	//return SlotIndex == TypedOther.SlotIndex;
	return true;
}
//---------------------------------------------------------------------

bool CSM30ConstMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
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
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CSM30RsrcMeta& TypedOther = (const CSM30RsrcMeta&)Other;
	return Register == TypedOther.Register;
}
//---------------------------------------------------------------------

bool CSM30SamplerMeta::IsEqual(const CMetadataObject& Other) const
{
	if (GetClass() != Other.GetClass() || GetShaderModel() != Other.GetShaderModel()) return false;
	const CSM30SamplerMeta& TypedOther = (const CSM30SamplerMeta&)Other;
	return Type == TypedOther.Type &&
		RegisterStart == TypedOther.RegisterStart &&
		RegisterCount == TypedOther.RegisterCount;
}
//---------------------------------------------------------------------

static void WriteRegisterRanges(const std::vector<size_t>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
{
	uint64_t RangeCountOffset = W.GetStream().GetPosition();
	W.Write<uint32_t>(0);

	if (!UsedRegs.size()) return;

	uint32_t RangeCount = 0;
	size_t CurrStart = UsedRegs[0], CurrCount = 1;
	for (size_t r = 1; r < UsedRegs.size(); ++r)
	{
		size_t Reg = UsedRegs[r];
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			W.Write<uint32_t>(CurrStart);
			W.Write<uint32_t>(CurrCount);
			++RangeCount;
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (size_t)-1)
	{
		W.Write<uint32_t>(CurrStart);
		W.Write<uint32_t>(CurrCount);
		++RangeCount;
	}

	uint64_t EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<uint32_t>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

static void ReadRegisterRanges(std::vector<size_t>& UsedRegs, IO::CBinaryReader& R)
{
	UsedRegs.Clear();

	uint32_t RangeCount = 0;
	R.Read<uint32_t>(RangeCount);
	for (size_t i = 0; i < RangeCount; ++i)
	{
		uint32_t Curr;
		uint32_t CurrCount;
		R.Read<uint32_t>(Curr);
		R.Read<uint32_t>(CurrCount);
		uint32_t CurrEnd = Curr + CurrCount;
		for (; Curr < CurrEnd; ++Curr)
			UsedRegs.Add((size_t)Curr);
	}
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::CollectFromBinaryAndSource(const void* pData, size_t Size, const char* pSource, size_t SourceSize, CDEMD3DInclude& IncludeHandler)
{
	std::vector<CD3D9ConstantDesc> D3D9Consts;
	std::map<uint32_t, CD3D9StructDesc> D3D9Structs;
	std::string Creator;

	if (!D3D9Reflect(pData, Size, D3D9Consts, D3D9Structs, Creator)) return false;

	// Process source code includes and remove comments

	std::string Source(pSource, SourceSize);

	//!!!DIRTY HACK! Need valid way to edit std::string inplace by StripComments()!
	size_t NewLength = StringUtils::StripComments(&Source[0]);
	Source.TruncateRight(Source.size() - NewLength);

	ptrdiff_t CurrIdx = 0;
	ptrdiff_t IncludeIdx;
	while ((IncludeIdx = Source.FindIndex("#include", CurrIdx)) != INVALID_INDEX)
	{
		ptrdiff_t FileNameStartSys = Source.FindIndex('<', IncludeIdx);
		ptrdiff_t FileNameStartLocal = Source.FindIndex('\"', IncludeIdx);
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
		ptrdiff_t FileNameStart;
		ptrdiff_t FileNameEnd;
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

		std::string IncludeFileName = Source.SubString(FileNameStart, FileNameEnd - FileNameStart);

		const void* pData;
		UINT Bytes;
		if (FAILED(IncludeHandler.Open(IncludeType, IncludeFileName.c_str(), nullptr, &pData, &Bytes)))
		{
			Messages += "SM30CollectShaderMetadata() > Failed to #include '";
			Messages += IncludeFileName;
			Messages += "', skipped\n";
			CurrIdx = FileNameEnd;
			continue;
		}

		Source =
			Source.SubString(0, IncludeIdx) +
			std::string((const char*)pData, Bytes) +
			Source.SubString(FileNameEnd + 1, Source.size() - FileNameEnd - 1);
		
		IncludeHandler.Close(pData);

		CurrIdx = IncludeIdx;

		//!!!DIRTY HACK! Need valid way to edit std::string inplace by StripComments()!
		NewLength = StringUtils::StripComments(&Source[0]);
		Source.TruncateRight(Source.size() - NewLength);
	}

	// Collect structure layout metadata

	const size_t StructCount = D3D9Structs.size();
	CSM30StructMeta* pStructMeta = Structs.Reserve(StructCount);
	for (size_t i = 0; i < StructCount; ++i, ++pStructMeta)
	{
		const CD3D9StructDesc& D3D9StructDesc = D3D9Structs.ValueAt(i);

		const size_t MemberCount = D3D9StructDesc.Members.size();
		CSM30StructMemberMeta* pMemberMeta = pStructMeta->Members.Reserve(MemberCount);
		for (size_t j = 0; j < MemberCount; ++j, ++pMemberMeta)
		{
			const CD3D9ConstantDesc& D3D9ConstDesc = D3D9StructDesc.Members[j];
			pMemberMeta->Name = D3D9ConstDesc.Name;
			pMemberMeta->StructIndex = (uint32_t)D3D9Structs.FindIndex(D3D9ConstDesc.StructID);
			pMemberMeta->RegisterOffset = D3D9ConstDesc.RegisterIndex;
			pMemberMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMemberMeta->ElementCount = D3D9ConstDesc.Type.Elements;
			pMemberMeta->Columns = D3D9ConstDesc.Type.Columns;
			pMemberMeta->Rows = D3D9ConstDesc.Type.Rows;
			pMemberMeta->Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				pMemberMeta->Flags |= ShaderConst_ColumnMajor;

			n_assert(pMemberMeta->ElementRegisterCount * pMemberMeta->ElementCount == D3D9ConstDesc.RegisterCount);
		}
	}

	// Collect constant metadata

	std::map<std::string, std::vector<std::string>> SampToTex;
	D3D9FindSamplerTextures(Source.c_str(), SampToTex);

	std::map<std::string, std::string> ConstToBuf;

	for (size_t i = 0; i < D3D9Consts.size(); ++i)
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

			size_t TexCount;
			int STIdx = SampToTex.FindIndex(D3D9ConstDesc.Name);
			if (STIdx == INVALID_INDEX) TexCount = 0;
			else
			{
				const std::vector<std::string>& TexNames = SampToTex.ValueAt(STIdx);
				TexCount = n_min(D3D9ConstDesc.RegisterCount, TexNames.size());
				for (size_t TexIdx = 0; TexIdx < TexCount; ++TexIdx)
				{
					const std::string& TexName = TexNames[TexIdx];
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
			std::string BufferName;
			uint32_t SlotIndex = (uint32_t)(INVALID_INDEX);
			D3D9FindConstantBuffer(Source.c_str(), D3D9ConstDesc.Name, BufferName, SlotIndex);

			size_t BufferIndex = 0;
			for (; BufferIndex < Buffers.size(); ++BufferIndex)
				if (Buffers[BufferIndex].Name == BufferName) break;

			if (BufferIndex == Buffers.size())
			{
				CSM30BufferMeta* pMeta = Buffers.Reserve(1);
				pMeta->Name = BufferName;
				pMeta->SlotIndex = SlotIndex;
			}
			else
			{
				CSM30BufferMeta& Meta = Buffers[BufferIndex];
				if (SlotIndex == (uint32_t)(INVALID_INDEX))
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
					return false;
				}
			}

			CSM30ConstMeta* pMeta = Consts.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->BufferIndex = BufferIndex;
			pMeta->StructIndex = (uint32_t)D3D9Structs.FindIndex(D3D9ConstDesc.StructID);

			switch (D3D9ConstDesc.RegisterSet)
			{
				case RS_FLOAT4:	pMeta->RegisterSet = RS_Float4; break;
				case RS_INT4:	pMeta->RegisterSet = RS_Int4; break;
				case RS_BOOL:	pMeta->RegisterSet = RS_Bool; break;
				default:
				{
					Sys::Error("Unsupported SM3.0 register set %d\n", D3D9ConstDesc.RegisterSet);
					return false;
				}
			};

			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMeta->ElementCount = D3D9ConstDesc.Type.Elements;
			pMeta->Columns = D3D9ConstDesc.Type.Columns;
			pMeta->Rows = D3D9ConstDesc.Type.Rows;
			pMeta->Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				pMeta->Flags |= ShaderConst_ColumnMajor;

			// Cache value
			pMeta->RegisterCount = pMeta->ElementRegisterCount * pMeta->ElementCount;

			n_assert(pMeta->RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30BufferMeta& BufMeta = Buffers[pMeta->BufferIndex];
			std::vector<size_t>& UsedRegs = (pMeta->RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 : ((pMeta->RegisterSet == RS_Int4) ? BufMeta.UsedInt4 : BufMeta.UsedBool);
			for (size_t r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				if (!UsedRegs.Contains(r)) UsedRegs.Add(r);
			}
		}
	}

	// Remove empty constant buffers and set free SlotIndices where no explicit value was specified

	std::vector<uint32_t> UsedSlotIndices;
	size_t i = 0;
	while (i < Buffers.size())
	{
		CSM30BufferMeta& B = Buffers[i];
		if (!B.UsedFloat4.size() &&
			!B.UsedInt4.size() &&
			!B.UsedBool.size())
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

	uint32_t NewSlotIndex = 0;
	for (size_t i = 0; i < Buffers.size(); ++i)
	{
		CSM30BufferMeta& B = Buffers[i];
		if (B.SlotIndex == (uint32_t)(INVALID_INDEX))
		{
			while (UsedSlotIndices.ContainsSorted(NewSlotIndex))
				++NewSlotIndex;
			B.SlotIndex = NewSlotIndex;
			++NewSlotIndex;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Save(IO::CBinaryWriter& W) const
{
	W.Write<uint32_t>(Buffers.size());
	for (size_t i = 0; i < Buffers.size(); ++i)
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

	W.Write<uint32_t>(Structs.size());
	for (size_t i = 0; i < Structs.size(); ++i)
	{
		CSM30StructMeta& Obj = Structs[i];

		W.Write<uint32_t>(Obj.Members.size());
		for (size_t j = 0; j < Obj.Members.size(); ++j)
		{
			CSM30StructMemberMeta& Member = Obj.Members[j];
			W.Write(Member.Name);
			W.Write(Member.StructIndex);
			W.Write(Member.RegisterOffset);
			W.Write(Member.ElementRegisterCount);
			W.Write(Member.ElementCount);
			W.Write(Member.Columns);
			W.Write(Member.Rows);
			W.Write(Member.Flags);
		}
	}

	W.Write<uint32_t>(Consts.size());
	for (size_t i = 0; i < Consts.size(); ++i)
	{
		const CSM30ConstMeta& Obj = Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.StructIndex);
		W.Write<uint8_t>(Obj.RegisterSet);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.ElementRegisterCount);
		W.Write(Obj.ElementCount);
		W.Write(Obj.Columns);
		W.Write(Obj.Rows);
		W.Write(Obj.Flags);
	}

	W.Write<uint32_t>(Resources.size());
	for (size_t i = 0; i < Resources.size(); ++i)
	{
		const CSM30RsrcMeta& Obj = Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
	}

	W.Write<uint32_t>(Samplers.size());
	for (size_t i = 0; i < Samplers.size(); ++i)
	{
		const CSM30SamplerMeta& Obj = Samplers[i];
		W.Write(Obj.Name);
		W.Write<uint8_t>(Obj.Type);
		W.Write(Obj.RegisterStart);
		W.Write(Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Load(IO::CBinaryReader& R)
{
	Buffers.Clear();
	Consts.Clear();
	Samplers.Clear();

	uint32_t Count;

	R.Read<uint32_t>(Count);
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

	R.Read<uint32_t>(Count);
	CSM30StructMeta* pStruct = Structs.Reserve(Count, false);
	for (; pStruct < Structs.End(); ++pStruct)
	{
		CSM30StructMeta& Obj = *pStruct;

		R.Read<uint32_t>(Count);
		CSM30StructMemberMeta* pMember = Obj.Members.Reserve(Count, false);
		for (; pMember < Obj.Members.End(); ++pMember)
		{
			CSM30StructMemberMeta& Member = *pMember;
			R.Read(Member.Name);
			R.Read(Member.StructIndex);
			R.Read(Member.RegisterOffset);
			R.Read(Member.ElementRegisterCount);
			R.Read(Member.ElementCount);
			R.Read(Member.Columns);
			R.Read(Member.Rows);
			R.Read(Member.Flags);
		}
	}

	R.Read<uint32_t>(Count);
	CSM30ConstMeta* pConst = Consts.Reserve(Count, false);
	for (; pConst < Consts.End(); ++pConst)
	{
		CSM30ConstMeta& Obj = *pConst;

		uint8_t RegSet;

		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.StructIndex);
		R.Read<uint8_t>(RegSet);

		Obj.RegisterSet = (ESM30RegisterSet)RegSet;

		R.Read(Obj.RegisterStart);
		R.Read(Obj.ElementRegisterCount);
		R.Read(Obj.ElementCount);
		R.Read(Obj.Columns);
		R.Read(Obj.Rows);
		R.Read(Obj.Flags);

		// Cache value
		Obj.RegisterCount = Obj.ElementRegisterCount * Obj.ElementCount;
	}

	R.Read<uint32_t>(Count);
	CSM30RsrcMeta* pRsrc = Resources.Reserve(Count, false);
	for (; pRsrc < Resources.End(); ++pRsrc)
	{
		CSM30RsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
	}

	R.Read<uint32_t>(Count);
	CSM30SamplerMeta* pSamp = Samplers.Reserve(Count, false);
	for (; pSamp < Samplers.End(); ++pSamp)
	{
		CSM30SamplerMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		
		uint8_t Type;
		R.Read<uint8_t>(Type);
		Obj.Type = (ESM30SamplerType)Type;
		
		R.Read(Obj.RegisterStart);
		R.Read(Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

uint32_t CSM30ShaderMeta::GetMinFeatureLevel() const
{
	return Render::GPU_Level_D3D9_3;
}
//---------------------------------------------------------------------

size_t CSM30ShaderMeta::GetParamCount(EShaderParamClass Class) const
{
	switch (Class)
	{
		case ShaderParam_Const:		return Consts.size();
		case ShaderParam_Resource:	return Resources.size();
		case ShaderParam_Sampler:	return Samplers.size();
		default:					return 0;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CSM30ShaderMeta::GetParamObject(EShaderParamClass Class, size_t Index)
{
	switch (Class)
	{
		case ShaderParam_Const:		return &Consts[Index];
		case ShaderParam_Resource:	return &Resources[Index];
		case ShaderParam_Sampler:	return &Samplers[Index];
		default:					return nullptr;
	}
}
//---------------------------------------------------------------------

size_t CSM30ShaderMeta::AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject)
{
	if (!pMetaObject || pMetaObject->GetShaderModel() != GetShaderModel() || pMetaObject->GetClass() != Class) return (size_t)(INVALID_INDEX);

	switch (Class)
	{
		case ShaderParam_Const:
		{
			Consts.Add(*(const CSM30ConstMeta*)pMetaObject);
			return Consts.size() - 1;
		}
		case ShaderParam_Resource:
		{
			Resources.Add(*(const CSM30RsrcMeta*)pMetaObject);
			return Resources.size() - 1;
		}
		case ShaderParam_Sampler:
		{
			Samplers.Add(*(const CSM30SamplerMeta*)pMetaObject);
			return Samplers.size() - 1;
		}
		default:	return (size_t)(INVALID_INDEX);
	}
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::FindParamObjectByName(EShaderParamClass Class, const char* pName, size_t& OutIndex) const
{
	switch (Class)
	{
		case ShaderParam_Const:
		{
			size_t Idx = 0;
			for (; Idx < Consts.size(); ++ Idx)
				if (Consts[Idx].Name == pName) break;
			if (Idx == Consts.size()) return false;
			OutIndex = Idx;
			return true;
		}
		case ShaderParam_Resource:
		{
			size_t Idx = 0;
			for (; Idx < Resources.size(); ++ Idx)
				if (Resources[Idx].Name == pName) break;
			if (Idx == Resources.size()) return false;
			OutIndex = Idx;
			return true;
		}
		case ShaderParam_Sampler:
		{
			size_t Idx = 0;
			for (; Idx < Samplers.size(); ++ Idx)
				if (Samplers[Idx].Name == pName) break;
			if (Idx == Samplers.size()) return false;
			OutIndex = Idx;
			return true;
		}
		default:					return false;
	}
}
//---------------------------------------------------------------------

size_t CSM30ShaderMeta::AddOrMergeBuffer(const CMetadataObject* pMetaBuffer)
{
	if (!pMetaBuffer || pMetaBuffer->GetShaderModel() != GetShaderModel()) return (size_t)(INVALID_INDEX);

	const CSM30BufferMeta* pSM30Buffer = (const CSM30BufferMeta*)pMetaBuffer;
	size_t Idx = 0;
	for (; Idx < Buffers.size(); ++ Idx)
		if (Buffers[Idx].SlotIndex == pSM30Buffer->SlotIndex) break;
	if (Idx == Buffers.size())
	{
		Buffers.Add(*pSM30Buffer);
		return Buffers.size() - 1;
	}
	else
	{
		// Merge used registers from the conflicting buffer
		
		CSM30BufferMeta& CurrBuffer = Buffers[Idx];
		const CSM30BufferMeta& NewBuffer = *pSM30Buffer;
		
		for (size_t r = 0; r < NewBuffer.UsedFloat4.size(); ++r)
		{
			size_t Register = NewBuffer.UsedFloat4[r];
			if (!CurrBuffer.UsedFloat4.Contains(Register))
				CurrBuffer.UsedFloat4.Add(Register);
		}
		CurrBuffer.UsedFloat4.Sort();
		
		for (size_t r = 0; r < NewBuffer.UsedInt4.size(); ++r)
		{
			size_t Register = NewBuffer.UsedInt4[r];
			if (!CurrBuffer.UsedInt4.Contains(Register))
				CurrBuffer.UsedInt4.Add(Register);
		}
		CurrBuffer.UsedInt4.Sort();
		
		for (size_t r = 0; r < NewBuffer.UsedBool.size(); ++r)
		{
			size_t Register = NewBuffer.UsedBool[r];
			if (!CurrBuffer.UsedBool.Contains(Register))
				CurrBuffer.UsedBool.Add(Register);
		}
		CurrBuffer.UsedBool.Sort();

		return Idx;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CSM30ShaderMeta::GetContainingConstantBuffer(const CMetadataObject* pMetaObject) const
{
	if (!pMetaObject || pMetaObject->GetClass() != ShaderParam_Const || pMetaObject->GetShaderModel() != GetShaderModel()) return nullptr;
	return &Buffers[((CSM30ConstMeta*)pMetaObject)->BufferIndex];
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::SetContainingConstantBuffer(size_t ConstIdx, size_t BufferIdx)
{
	if (ConstIdx >= Consts.size()) return false;
	Consts[ConstIdx].BufferIndex = BufferIdx;
	return true;
}
//---------------------------------------------------------------------

uint32_t CSM30ShaderMeta::AddStructure(const CShaderMetadata& SourceMeta, uint64_t StructKey, std::map<uint64_t, uint32_t>& StructIndexMapping)
{
	ptrdiff_t StructIdxIdx = StructIndexMapping.FindIndex(StructKey);
	if (StructIdxIdx != INVALID_INDEX) return StructIndexMapping.ValueAt(StructIdxIdx);

	const uint32_t StructIndex = (uint32_t)(StructKey & 0xffffffff);
	const CSM30StructMeta& StructMeta = ((const CSM30ShaderMeta&)SourceMeta).Structs[StructIndex];
	Structs.Add(StructMeta);
	StructIndexMapping.Add(StructKey, Structs.size() - 1);

	for (size_t i = 0; i < StructMeta.Members.size(); ++i)
	{
		const CSM30StructMemberMeta& MemberMeta = StructMeta.Members[i];
		if (MemberMeta.StructIndex == (uint32_t)(-1)) continue;

		const uint64_t MemberKey = (StructKey & 0xffffffff00000000) | ((uint64_t)MemberMeta.StructIndex);
		AddStructure(SourceMeta, MemberKey, StructIndexMapping);
	}

	StructIdxIdx = StructIndexMapping.FindIndex(StructKey);
	n_assert(StructIdxIdx != INVALID_INDEX);
	return StructIndexMapping.ValueAt(StructIdxIdx);
}
//---------------------------------------------------------------------

uint32_t CSM30ShaderMeta::GetStructureIndex(size_t ConstIdx) const
{
	if (ConstIdx >= Consts.size()) return (uint32_t)(-1);
	return Consts[ConstIdx].StructIndex;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::SetStructureIndex(size_t ConstIdx, uint32_t StructIdx)
{
	if (ConstIdx >= Consts.size()) return false;
	Consts[ConstIdx].StructIndex = StructIdx;
	return true;
}
//---------------------------------------------------------------------
