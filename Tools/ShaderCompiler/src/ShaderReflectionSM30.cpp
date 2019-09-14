#include "ShaderReflectionSM30.h"

#include <DEMD3DInclude.h>
#include <D3D9ShaderReflectionAPI.h>
#include <algorithm>
#include <cassert>

#undef min
#undef max

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

static void WriteRegisterRanges(const std::set<uint32_t>& UsedRegs, std::ofstream& File, const char* pRegisterSetName)
{
	uint64_t RangeCountOffset = File.tellp();
	WriteFile<uint32_t>(File, 0);

	if (!UsedRegs.size()) return;

	auto It = UsedRegs.cbegin();

	uint32_t RangeCount = 0;
	uint32_t CurrStart = *It++;
	uint32_t CurrCount = 1;
	for (; It != UsedRegs.cend(); ++It)
	{
		const uint32_t Reg = *It;
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			WriteFile<uint32_t>(File, CurrStart);
			WriteFile<uint32_t>(File, CurrCount);
			++RangeCount;
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	// The last range
	WriteFile<uint32_t>(File, CurrStart);
	WriteFile<uint32_t>(File, CurrCount);
	++RangeCount;

	uint64_t EndOffset = File.tellp();
	File.seekp(RangeCountOffset, std::ios_base::beg);
	WriteFile<uint32_t>(File, RangeCount);
	File.seekp(EndOffset, std::ios_base::beg);
}
//---------------------------------------------------------------------

static void ReadRegisterRanges(std::set<uint32_t>& UsedRegs, std::ifstream& File)
{
	UsedRegs.clear();

	uint32_t RangeCount = 0;
	ReadFile<uint32_t>(File, RangeCount);
	for (uint32_t i = 0; i < RangeCount; ++i)
	{
		uint32_t Curr;
		uint32_t CurrCount;
		ReadFile<uint32_t>(File, Curr);
		ReadFile<uint32_t>(File, CurrCount);
		uint32_t CurrEnd = Curr + CurrCount;
		for (; Curr < CurrEnd; ++Curr)
			UsedRegs.emplace(Curr);
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

	size_t CurrIdx = 0;
	size_t IncludeIdx;
	while ((IncludeIdx = Source.find("#include", CurrIdx)) != std::string::npos)
	{
		auto FileNameStartSys = Source.find('<', IncludeIdx);
		auto FileNameStartLocal = Source.find('\"', IncludeIdx);
		if (FileNameStartSys == std::string::npos && FileNameStartLocal == std::string::npos)
		{
			Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
			break;
		}

		if (FileNameStartSys != std::string::npos && FileNameStartLocal != std::string::npos)
		{
			if (FileNameStartSys < FileNameStartLocal) FileNameStartLocal = std::string::npos;
			else FileNameStartSys = std::string::npos;
		}

		D3D_INCLUDE_TYPE IncludeType;
		size_t FileNameStart;
		size_t FileNameEnd;
		if (FileNameStartSys != std::string::npos)
		{
			IncludeType = D3D_INCLUDE_SYSTEM; // <FileName>
			FileNameStart = FileNameStartSys + 1;
			FileNameEnd = Source.find('>', FileNameStart);
		}
		else
		{
			IncludeType = D3D_INCLUDE_LOCAL; // "FileName"
			FileNameStart = FileNameStartLocal + 1;
			FileNameEnd = Source.find('\"', FileNameStart);
		}

		if (FileNameEnd == std::string::npos)
		{
			Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
			break;
		}

		std::string IncludeFileName = Source.substr(FileNameStart, FileNameEnd - FileNameStart);

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
			Source.substr(0, IncludeIdx) +
			std::string((const char*)pData, Bytes) +
			Source.substr(FileNameEnd + 1, Source.size() - FileNameEnd - 1);
		
		IncludeHandler.Close(pData);

		CurrIdx = IncludeIdx;

		//!!!DIRTY HACK! Need valid way to edit std::string inplace by StripComments()!
		NewLength = StringUtils::StripComments(&Source[0]);
		Source.TruncateRight(Source.size() - NewLength);
	}

	// Collect structure layout metadata

	Structs.reserve(D3D9Structs.size());
	for (const auto& Pair : D3D9Structs)
	{
		const CD3D9StructDesc& D3D9StructDesc = Pair.second;

		CSM30StructMeta StructMeta;

		StructMeta.Members.reserve(D3D9StructDesc.Members.size());
		for (const auto& D3D9ConstDesc : D3D9StructDesc.Members)
		{
			CSM30StructMemberMeta MemberMeta;

			MemberMeta.Name = D3D9ConstDesc.Name;
			MemberMeta.StructIndex = (uint32_t)D3D9Structs.FindIndex(D3D9ConstDesc.StructID);
			MemberMeta.RegisterOffset = D3D9ConstDesc.RegisterIndex;
			MemberMeta.ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			MemberMeta.ElementCount = D3D9ConstDesc.Type.Elements;
			MemberMeta.Columns = static_cast<uint8_t>(D3D9ConstDesc.Type.Columns);
			MemberMeta.Rows = static_cast<uint8_t>(D3D9ConstDesc.Type.Rows);
			MemberMeta.Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				MemberMeta.Flags |= ShaderConst_ColumnMajor;

			assert(MemberMeta.ElementRegisterCount * MemberMeta.ElementCount == D3D9ConstDesc.RegisterCount);

			StructMeta.Members.push_back(std::move(MemberMeta));
		}

		Structs.push_back(std::move(StructMeta));
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
				CSM30BufferMeta Meta;
				Meta.Name = BufferName;
				Meta.SlotIndex = SlotIndex;
				Buffers.push_back(std::move(Meta));
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
					Messages += std::to_string(SlotIndex);
					Messages += " and ";
					Messages += std::to_string(SlotIndex);
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
					// FIXME message!
					assert(false && "Unsupported SM3.0 register set %d\n");//, D3D9ConstDesc.RegisterSet);
					return false;
				}
			};

			pMeta->RegisterStart = D3D9ConstDesc.RegisterIndex;
			pMeta->ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			pMeta->ElementCount = D3D9ConstDesc.Type.Elements;
			pMeta->Columns = static_cast<uint8_t>(D3D9ConstDesc.Type.Columns);
			pMeta->Rows = static_cast<uint8_t>(D3D9ConstDesc.Type.Rows);
			pMeta->Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				pMeta->Flags |= ShaderConst_ColumnMajor;

			// Cache value
			pMeta->RegisterCount = pMeta->ElementRegisterCount * pMeta->ElementCount;

			assert(pMeta->RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30BufferMeta& BufMeta = Buffers[pMeta->BufferIndex];
			auto& UsedRegs =
				(pMeta->RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 :
				((pMeta->RegisterSet == RS_Int4) ? BufMeta.UsedInt4 :
					BufMeta.UsedBool);
			for (uint32_t r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				UsedRegs.emplace(r);
			}
		}
	}

	// Remove empty constant buffers and assign free slots to buffers for which no explicit value was specified

	Buffers.erase(std::remove_if(Buffers.begin(), Buffers.end(), [](const CSM30BufferMeta& Buffer)
	{
		return !Buffer.UsedFloat4.size() && !Buffer.UsedInt4.size() && !Buffer.UsedBool.size();
	}), Buffers.end());

	std::set<uint32_t> UsedSlotIndices;
	for (const auto& Buffer : Buffers)
		UsedSlotIndices.emplace(Buffer.SlotIndex);

	uint32_t NewSlotIndex = 0;
	for (size_t i = 0; i < Buffers.size(); ++i)
	{
		CSM30BufferMeta& Buffer = Buffers[i];
		if (Buffer.SlotIndex == (uint32_t)(-1))
		{
			while (UsedSlotIndices.find(NewSlotIndex) != UsedSlotIndices.cend())
				++NewSlotIndex;
			Buffer.SlotIndex = NewSlotIndex;
			++NewSlotIndex;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Save(std::ofstream& File) const
{
	WriteFile<uint32_t>(File, Buffers.size());
	for (const auto& Obj : Buffers)
	{
		WriteFile(File, Obj.Name);
		WriteFile(File, Obj.SlotIndex);

		WriteRegisterRanges(Obj.UsedFloat4, File, "float4");
		WriteRegisterRanges(Obj.UsedInt4, File, "int4");
		WriteRegisterRanges(Obj.UsedBool, File, "bool");
	}

	WriteFile<uint32_t>(File, Structs.size());
	for (const auto& Obj : Structs)
	{
		WriteFile<uint32_t>(File, Obj.Members.size());
		for (const auto& Member : Obj.Members)
		{
			WriteFile(File, Member.Name);
			WriteFile(File, Member.StructIndex);
			WriteFile(File, Member.RegisterOffset);
			WriteFile(File, Member.ElementRegisterCount);
			WriteFile(File, Member.ElementCount);
			WriteFile(File, Member.Columns);
			WriteFile(File, Member.Rows);
			WriteFile(File, Member.Flags);
		}
	}

	WriteFile<uint32_t>(File, Consts.size());
	for (const auto& Obj : Consts)
	{
		WriteFile(File, Obj.Name);
		WriteFile(File, Obj.BufferIndex);
		WriteFile(File, Obj.StructIndex);
		WriteFile<uint8_t>(File, Obj.RegisterSet);
		WriteFile(File, Obj.RegisterStart);
		WriteFile(File, Obj.ElementRegisterCount);
		WriteFile(File, Obj.ElementCount);
		WriteFile(File, Obj.Columns);
		WriteFile(File, Obj.Rows);
		WriteFile(File, Obj.Flags);
	}

	WriteFile<uint32_t>(File, Resources.size());
	for (const auto& Obj : Resources)
	{
		WriteFile(File, Obj.Name);
		WriteFile(File, Obj.Register);
	}

	WriteFile<uint32_t>(File, Samplers.size());
	for (const auto& Obj : Samplers)
	{
		WriteFile(File, Obj.Name);
		WriteFile<uint8_t>(File, Obj.Type);
		WriteFile(File, Obj.RegisterStart);
		WriteFile(File, Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Load(std::ifstream& File)
{
	Buffers.clear();
	Consts.clear();
	Samplers.clear();

	uint32_t Count;

	ReadFile<uint32_t>(File, Count);
	Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30BufferMeta Obj;

		ReadFile(File, Obj.Name);
		ReadFile(File, Obj.SlotIndex);

		ReadRegisterRanges(Obj.UsedFloat4, File);
		ReadRegisterRanges(Obj.UsedInt4, File);
		ReadRegisterRanges(Obj.UsedBool, File);

		Buffers.push_back(std::move(Obj));
	}

	ReadFile<uint32_t>(File, Count);
	Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30StructMeta Obj;

		uint32_t MemberCount;
		ReadFile<uint32_t>(File, MemberCount);
		Obj.Members.reserve(MemberCount);
		for (uint32_t j = 0; j < MemberCount; ++j)
		{
			CSM30StructMemberMeta Member;
			ReadFile(File, Member.Name);
			ReadFile(File, Member.StructIndex);
			ReadFile(File, Member.RegisterOffset);
			ReadFile(File, Member.ElementRegisterCount);
			ReadFile(File, Member.ElementCount);
			ReadFile(File, Member.Columns);
			ReadFile(File, Member.Rows);
			ReadFile(File, Member.Flags);

			Obj.Members.push_back(std::move(Member));
		}

		Structs.push_back(std::move(Obj));
	}

	ReadFile<uint32_t>(File, Count);
	Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30ConstMeta Obj;

		ReadFile(File, Obj.Name);
		ReadFile(File, Obj.BufferIndex);
		ReadFile(File, Obj.StructIndex);

		uint8_t RegSet;
		ReadFile<uint8_t>(File, RegSet);
		Obj.RegisterSet = (ESM30RegisterSet)RegSet;

		ReadFile(File, Obj.RegisterStart);
		ReadFile(File, Obj.ElementRegisterCount);
		ReadFile(File, Obj.ElementCount);
		ReadFile(File, Obj.Columns);
		ReadFile(File, Obj.Rows);
		ReadFile(File, Obj.Flags);

		// Cache value
		Obj.RegisterCount = Obj.ElementRegisterCount * Obj.ElementCount;

		Consts.push_back(std::move(Obj));
	}

	ReadFile<uint32_t>(File, Count);
	Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30RsrcMeta Obj;
		ReadFile(File, Obj.Name);
		ReadFile(File, Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
		Resources.push_back(std::move(Obj));
	}

	ReadFile<uint32_t>(File, Count);
	Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30SamplerMeta Obj;
		ReadFile(File, Obj.Name);
		
		uint8_t Type;
		ReadFile<uint8_t>(File, Type);
		Obj.Type = (ESM30SamplerType)Type;
		
		ReadFile(File, Obj.RegisterStart);
		ReadFile(File, Obj.RegisterCount);

		Samplers.push_back(std::move(Obj));
	}

	return true;
}
//---------------------------------------------------------------------

uint32_t CSM30ShaderMeta::GetMinFeatureLevel() const
{
	return GPU_Level_D3D9_3;
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
	if (!pMetaObject || pMetaObject->GetShaderModel() != GetShaderModel() || pMetaObject->GetClass() != Class)
		return std::numeric_limits<size_t>().max();

	switch (Class)
	{
		case ShaderParam_Const:
		{
			Consts.push_back(*(const CSM30ConstMeta*)pMetaObject);
			return Consts.size() - 1;
		}
		case ShaderParam_Resource:
		{
			Resources.push_back(*(const CSM30RsrcMeta*)pMetaObject);
			return Resources.size() - 1;
		}
		case ShaderParam_Sampler:
		{
			Samplers.push_back(*(const CSM30SamplerMeta*)pMetaObject);
			return Samplers.size() - 1;
		}
		default: return std::numeric_limits<size_t>().max();
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
	if (!pMetaBuffer || pMetaBuffer->GetShaderModel() != GetShaderModel())
		return std::numeric_limits<size_t>().max();

	const CSM30BufferMeta* pSM30Buffer = (const CSM30BufferMeta*)pMetaBuffer;
	size_t Idx = 0;
	for (; Idx < Buffers.size(); ++ Idx)
		if (Buffers[Idx].SlotIndex == pSM30Buffer->SlotIndex) break;
	if (Idx == Buffers.size())
	{
		Buffers.push_back(*pSM30Buffer);
		return Buffers.size() - 1;
	}
	else
	{
		// Merge used registers from the conflicting buffer
		
		CSM30BufferMeta& CurrBuffer = Buffers[Idx];
		const CSM30BufferMeta& NewBuffer = *pSM30Buffer;
		
		CurrBuffer.UsedFloat4.insert(NewBuffer.UsedFloat4.begin(), NewBuffer.UsedFloat4.end());
		CurrBuffer.UsedInt4.insert(NewBuffer.UsedInt4.begin(), NewBuffer.UsedInt4.end());
		CurrBuffer.UsedBool.insert(NewBuffer.UsedBool.begin(), NewBuffer.UsedBool.end());

		return Idx;
	}
}
//---------------------------------------------------------------------

CMetadataObject* CSM30ShaderMeta::GetContainingConstantBuffer(const CMetadataObject* pMetaObject)
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
	auto It = StructIndexMapping.find(StructKey);
	if (It != StructIndexMapping.cend()) return It->second;

	const uint32_t StructIndex = (uint32_t)(StructKey & 0xffffffff);
	const CSM30StructMeta& StructMeta = ((const CSM30ShaderMeta&)SourceMeta).Structs[StructIndex];
	Structs.push_back(StructMeta);
	StructIndexMapping.emplace(StructKey, Structs.size() - 1);

	for (size_t i = 0; i < StructMeta.Members.size(); ++i)
	{
		const CSM30StructMemberMeta& MemberMeta = StructMeta.Members[i];
		if (MemberMeta.StructIndex == (uint32_t)(-1)) continue;

		const uint64_t MemberKey = (StructKey & 0xffffffff00000000) | ((uint64_t)MemberMeta.StructIndex);
		AddStructure(SourceMeta, MemberKey, StructIndexMapping);
	}

	It = StructIndexMapping.find(StructKey);
	assert(It != StructIndexMapping.cend());
	return It->second;
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
