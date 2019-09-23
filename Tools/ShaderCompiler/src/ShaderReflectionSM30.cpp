#include "ShaderReflectionSM30.h"

#include <DEMD3DInclude.h>
#include <D3D9ShaderReflectionAPI.h>
#include <Utils.h>
#include <cassert>

#undef min
#undef max

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

static void WriteRegisterRanges(const std::set<uint32_t>& UsedRegs, std::ostream& Stream, const char* pRegisterSetName)
{
	uint64_t RangeCountOffset = Stream.tellp();
	WriteStream<uint32_t>(Stream, 0);

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
			WriteStream<uint32_t>(Stream, CurrStart);
			WriteStream<uint32_t>(Stream, CurrCount);
			++RangeCount;
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	// The last range
	WriteStream<uint32_t>(Stream, CurrStart);
	WriteStream<uint32_t>(Stream, CurrCount);
	++RangeCount;

	uint64_t EndOffset = Stream.tellp();
	Stream.seekp(RangeCountOffset, std::ios_base::beg);
	WriteStream<uint32_t>(Stream, RangeCount);
	Stream.seekp(EndOffset, std::ios_base::beg);
}
//---------------------------------------------------------------------

static void ReadRegisterRanges(std::set<uint32_t>& UsedRegs, std::istream& Stream)
{
	UsedRegs.clear();

	uint32_t RangeCount = 0;
	ReadStream<uint32_t>(Stream, RangeCount);
	for (uint32_t i = 0; i < RangeCount; ++i)
	{
		uint32_t Curr;
		uint32_t CurrCount;
		ReadStream<uint32_t>(Stream, Curr);
		ReadStream<uint32_t>(Stream, CurrCount);
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

	// Remove comments from the source code

	// TODO: try D3DPreprocess, try D3D_COMPILE_STANDARD_FILE_INCLUDE!

	std::string Source;
	{
		std::unique_ptr<char[]> pSourceCopy(new char[SourceSize + 1]);
		memcpy(pSourceCopy.get(), pSource, SourceSize);
		pSourceCopy[SourceSize] = 0;

		const size_t NewLength = StripComments(pSourceCopy.get());
		Source.assign(pSourceCopy.get(), NewLength);
	}

	// Insert includes right into a code, as preprocessor does

	size_t CurrIdx = 0;
	size_t IncludeIdx;
	while ((IncludeIdx = Source.find("#include", CurrIdx)) != std::string::npos)
	{
		auto FileNameStartSys = Source.find('<', IncludeIdx);
		auto FileNameStartLocal = Source.find('\"', IncludeIdx);
		if (FileNameStartSys == std::string::npos && FileNameStartLocal == std::string::npos)
		{
			//Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
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
			//Messages += "SM30CollectShaderMetadata() > Invalid #include in a shader source code\n";
			break;
		}

		std::string IncludeFileName = Source.substr(FileNameStart, FileNameEnd - FileNameStart);

		const void* pIncludeSource;
		UINT IncludeSourceSize;
		if (FAILED(IncludeHandler.Open(IncludeType, IncludeFileName.c_str(), nullptr, &pIncludeSource, &IncludeSourceSize)))
		{
			//Messages += "SM30CollectShaderMetadata() > Failed to #include '";
			//Messages += IncludeFileName;
			//Messages += "', skipped\n";
			CurrIdx = FileNameEnd;
			continue;
		}

		// Remove comments from the obtained include file
		std::string IncludeSource;
		{
			std::unique_ptr<char[]> pSourceCopy(new char[IncludeSourceSize + 1]);
			memcpy(pSourceCopy.get(), pIncludeSource, IncludeSourceSize);
			pSourceCopy[SourceSize] = 0;

			const size_t NewLength = StripComments(pSourceCopy.get());
			IncludeSource.assign(pSourceCopy.get(), NewLength);
		}

		IncludeHandler.Close(pIncludeSource);

		Source =
			Source.substr(0, IncludeIdx) + ' ' +
			IncludeSource + ' ' +
			Source.substr(FileNameEnd + 1);

		// Start from the place where include was inserted, because it can
		// contain another include right at the start
		CurrIdx = IncludeIdx;
	}

	// Collect structure layout metadata

	// Since D3D9Structs is a map, its elements have no indices. We generate linear indices for them.
	// These indices are valid to reference structs in a Structs vector.
	std::map<uint32_t, uint32_t> StructIDToIndex;
	size_t Idx = 0;
	for (const auto& Pair : D3D9Structs)
		StructIDToIndex.emplace(Pair.first, Idx++);

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
			MemberMeta.StructIndex = StructIDToIndex[D3D9ConstDesc.StructID];
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
			//Messages += "    SM3.0 mixed-regset structs aren't supported, '";
			//Messages += D3D9ConstDesc.Name;
			//Messages += "' skipped\n";
			continue;
		}
		else if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CSM30SamplerMeta Meta;
			Meta.Name = D3D9ConstDesc.Name;
			Meta.RegisterStart = D3D9ConstDesc.RegisterIndex;
			Meta.RegisterCount = D3D9ConstDesc.RegisterCount;

			switch (D3D9ConstDesc.Type.Type)
			{
				case PT_SAMPLER1D:		Meta.Type = SM30Sampler_1D; break;
				case PT_SAMPLER3D:		Meta.Type = SM30Sampler_3D; break;
				case PT_SAMPLERCUBE:	Meta.Type = SM30Sampler_CUBE; break;
				default:				Meta.Type = SM30Sampler_2D; break;
			}

			size_t TexCount;
			auto STIt = SampToTex.find(D3D9ConstDesc.Name);
			if (STIt == SampToTex.cend()) TexCount = 0;
			else
			{
				const std::vector<std::string>& TexNames = STIt->second;
				TexCount = std::min(D3D9ConstDesc.RegisterCount, TexNames.size());
				for (size_t TexIdx = 0; TexIdx < TexCount; ++TexIdx)
				{
					const std::string& TexName = TexNames[TexIdx];
					if (!TexName.empty())
					{
						CSM30RsrcMeta Meta;
						Meta.Name = TexName;
						Meta.Register = D3D9ConstDesc.RegisterIndex + TexIdx;
						Resources.push_back(std::move(Meta));
					}
					else if (D3D9ConstDesc.RegisterCount > 1)
					{
						//Messages += "Sampler '";
						//Messages += D3D9ConstDesc.Name;
						//Messages += '[';
						//Messages += std::to_string(TexIdx);
						//Messages += "]' has no texture bound, use initializer in a form of 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
					}
					else
					{
						//Messages += "Sampler '";
						//Messages += D3D9ConstDesc.Name;
						//Messages += "' has no texture bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'\n";
					}
				}
			}

			if (!TexCount)
			{
				//Messages += "Sampler '";
				//Messages += D3D9ConstDesc.Name;
				//Messages += "' has no textures bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }' or 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'\n";
			}

			Samplers.push_back(std::move(Meta));
		}
		else // Constants
		{
			std::string BufferName;
			uint32_t SlotIndex = (uint32_t)(-1);
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
				if (SlotIndex == (uint32_t)(-1))
					SlotIndex = SlotIndex;
				else if (SlotIndex != SlotIndex)
				{
					//Messages += "CBuffer '";
					//Messages += Meta.Name;
					//Messages += "' is bound to different SlotIndex values (at least ";
					//Messages += std::to_string(SlotIndex);
					//Messages += " and ";
					//Messages += std::to_string(SlotIndex);
					//Messages += ") in the same shader, please fix it\n";
					return false;
				}
			}

			CSM30ConstMeta Meta;
			Meta.Name = D3D9ConstDesc.Name;
			Meta.BufferIndex = BufferIndex;
			Meta.StructIndex = StructIDToIndex[D3D9ConstDesc.StructID];

			switch (D3D9ConstDesc.RegisterSet)
			{
				case RS_FLOAT4:	Meta.RegisterSet = RS_Float4; break;
				case RS_INT4:	Meta.RegisterSet = RS_Int4; break;
				case RS_BOOL:	Meta.RegisterSet = RS_Bool; break;
				default:
				{
					// FIXME message!
					assert(false && "Unsupported SM3.0 register set %d\n");//, D3D9ConstDesc.RegisterSet);
					return false;
				}
			};

			Meta.RegisterStart = D3D9ConstDesc.RegisterIndex;
			Meta.ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			Meta.ElementCount = D3D9ConstDesc.Type.Elements;
			Meta.Columns = static_cast<uint8_t>(D3D9ConstDesc.Type.Columns);
			Meta.Rows = static_cast<uint8_t>(D3D9ConstDesc.Type.Rows);
			Meta.Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				Meta.Flags |= ShaderConst_ColumnMajor;

			// Cache value
			Meta.RegisterCount = Meta.ElementRegisterCount * Meta.ElementCount;

			assert(Meta.RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30BufferMeta& BufMeta = Buffers[Meta.BufferIndex];
			auto& UsedRegs =
				(Meta.RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 :
				((Meta.RegisterSet == RS_Int4) ? BufMeta.UsedInt4 :
					BufMeta.UsedBool);
			for (uint32_t r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				UsedRegs.emplace(r);
			}

			Consts.push_back(std::move(Meta));
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

bool CSM30ShaderMeta::Save(std::ostream& Stream) const
{
	WriteStream<uint32_t>(Stream, Buffers.size());
	for (const auto& Obj : Buffers)
	{
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.SlotIndex);

		WriteRegisterRanges(Obj.UsedFloat4, Stream, "float4");
		WriteRegisterRanges(Obj.UsedInt4, Stream, "int4");
		WriteRegisterRanges(Obj.UsedBool, Stream, "bool");
	}

	WriteStream<uint32_t>(Stream, Structs.size());
	for (const auto& Obj : Structs)
	{
		WriteStream<uint32_t>(Stream, Obj.Members.size());
		for (const auto& Member : Obj.Members)
		{
			WriteStream(Stream, Member.Name);
			WriteStream(Stream, Member.StructIndex);
			WriteStream(Stream, Member.RegisterOffset);
			WriteStream(Stream, Member.ElementRegisterCount);
			WriteStream(Stream, Member.ElementCount);
			WriteStream(Stream, Member.Columns);
			WriteStream(Stream, Member.Rows);
			WriteStream(Stream, Member.Flags);
		}
	}

	WriteStream<uint32_t>(Stream, Consts.size());
	for (const auto& Obj : Consts)
	{
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.BufferIndex);
		WriteStream(Stream, Obj.StructIndex);
		WriteStream<uint8_t>(Stream, Obj.RegisterSet);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.ElementRegisterCount);
		WriteStream(Stream, Obj.ElementCount);
		WriteStream(Stream, Obj.Columns);
		WriteStream(Stream, Obj.Rows);
		WriteStream(Stream, Obj.Flags);
	}

	WriteStream<uint32_t>(Stream, Resources.size());
	for (const auto& Obj : Resources)
	{
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.Register);
	}

	WriteStream<uint32_t>(Stream, Samplers.size());
	for (const auto& Obj : Samplers)
	{
		WriteStream(Stream, Obj.Name);
		WriteStream<uint8_t>(Stream, Obj.Type);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CSM30ShaderMeta::Load(std::istream& Stream)
{
	Buffers.clear();
	Consts.clear();
	Samplers.clear();

	uint32_t Count;

	ReadStream<uint32_t>(Stream, Count);
	Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30BufferMeta Obj;

		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.SlotIndex);

		ReadRegisterRanges(Obj.UsedFloat4, Stream);
		ReadRegisterRanges(Obj.UsedInt4, Stream);
		ReadRegisterRanges(Obj.UsedBool, Stream);

		Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30StructMeta Obj;

		uint32_t MemberCount;
		ReadStream<uint32_t>(Stream, MemberCount);
		Obj.Members.reserve(MemberCount);
		for (uint32_t j = 0; j < MemberCount; ++j)
		{
			CSM30StructMemberMeta Member;
			ReadStream(Stream, Member.Name);
			ReadStream(Stream, Member.StructIndex);
			ReadStream(Stream, Member.RegisterOffset);
			ReadStream(Stream, Member.ElementRegisterCount);
			ReadStream(Stream, Member.ElementCount);
			ReadStream(Stream, Member.Columns);
			ReadStream(Stream, Member.Rows);
			ReadStream(Stream, Member.Flags);

			Obj.Members.push_back(std::move(Member));
		}

		Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30ConstMeta Obj;

		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.BufferIndex);
		ReadStream(Stream, Obj.StructIndex);

		uint8_t RegSet;
		ReadStream<uint8_t>(Stream, RegSet);
		Obj.RegisterSet = (ESM30RegisterSet)RegSet;

		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.ElementRegisterCount);
		ReadStream(Stream, Obj.ElementCount);
		ReadStream(Stream, Obj.Columns);
		ReadStream(Stream, Obj.Rows);
		ReadStream(Stream, Obj.Flags);

		// Cache value
		Obj.RegisterCount = Obj.ElementRegisterCount * Obj.ElementCount;

		Consts.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30RsrcMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.Register);
		//???store sampler type or index for texture type validation on set?
		//???how to reference texture object in SM3.0 shader for it to be included in params list?
		Resources.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30SamplerMeta Obj;
		ReadStream(Stream, Obj.Name);
		
		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Obj.Type = (ESM30SamplerType)Type;
		
		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.RegisterCount);

		Samplers.push_back(std::move(Obj));
	}

	return true;
}
//---------------------------------------------------------------------
