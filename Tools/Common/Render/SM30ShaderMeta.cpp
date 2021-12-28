#include "SM30ShaderMeta.h"
#include <Utils.h>

static void BuildRegisterRanges(const std::set<uint32_t>& UsedRegs, std::vector<std::pair<uint32_t, uint32_t>>& Out)
{
	if (!UsedRegs.size()) return;

	auto It = UsedRegs.cbegin();

	uint32_t CurrStart = *It++;
	uint32_t CurrCount = 1;
	for (; It != UsedRegs.cend(); ++It)
	{
		const uint32_t Reg = *It;
		if (Reg == CurrStart + CurrCount)
			++CurrCount;
		else
		{
			// New range detected
			Out.emplace_back(CurrStart, CurrCount);
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	// The last range
	Out.emplace_back(CurrStart, CurrCount);
}

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

std::ostream& operator <<(std::ostream& Stream, const CSM30BufferMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.SlotIndex);

	std::vector<const std::set<uint32_t>*> RegSets = { &Value.UsedFloat4, &Value.UsedInt4, &Value.UsedBool };
	for (const auto* pRegSet : RegSets)
	{
		std::vector<std::pair<uint32_t, uint32_t>> Ranges;
		BuildRegisterRanges(*pRegSet, Ranges);

		WriteStream<uint32_t>(Stream, Ranges.size());
		for (const auto& Range : Ranges)
		{
			WriteStream<uint32_t>(Stream, Range.first);
			WriteStream<uint32_t>(Stream, Range.second);
		}
	}

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30BufferMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.SlotIndex);

	ReadRegisterRanges(Value.UsedFloat4, Stream);
	ReadRegisterRanges(Value.UsedInt4, Stream);
	ReadRegisterRanges(Value.UsedBool, Stream);

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30StructMeta& Value)
{
	WriteStream<uint8_t>(Stream, Value.RegisterSet);

	WriteStream<uint32_t>(Stream, Value.Members.size());
	for (const auto& Member : Value.Members)
	{
		WriteStream(Stream, Member.Name);
		WriteStream(Stream, Member.StructIndex);
		// NB: Member.RegisterSet is not saved because it must be the same for all struct members
		WriteStream(Stream, Member.RegisterStart);
		WriteStream(Stream, Member.ElementRegisterCount);
		WriteStream(Stream, Member.ElementCount);
		WriteStream(Stream, Member.Columns);
		WriteStream(Stream, Member.Rows);
		WriteStream(Stream, Member.Flags);
	}

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30StructMeta& Value)
{
	Value.RegisterSet = static_cast<ESM30RegisterSet>(ReadStream<uint8_t>(Stream));

	uint32_t MemberCount;
	ReadStream<uint32_t>(Stream, MemberCount);
	Value.Members.reserve(MemberCount);
	for (uint32_t j = 0; j < MemberCount; ++j)
	{
		CSM30ConstMetaBase Member;
		ReadStream(Stream, Member.Name);
		ReadStream(Stream, Member.StructIndex);
		ReadStream(Stream, Member.RegisterStart);
		ReadStream(Stream, Member.ElementRegisterCount);
		ReadStream(Stream, Member.ElementCount);
		ReadStream(Stream, Member.Columns);
		ReadStream(Stream, Member.Rows);
		ReadStream(Stream, Member.Flags);

		Value.Members.push_back(std::move(Member));
	}

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30ConstMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.BufferIndex);
	WriteStream(Stream, Value.StructIndex);
	WriteStream<uint8_t>(Stream, Value.RegisterSet);
	WriteStream(Stream, Value.RegisterStart);
	WriteStream(Stream, Value.ElementRegisterCount);
	WriteStream(Stream, Value.ElementCount);
	WriteStream(Stream, Value.Columns);
	WriteStream(Stream, Value.Rows);
	WriteStream(Stream, Value.Flags);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30ConstMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.BufferIndex);
	ReadStream(Stream, Value.StructIndex);

	Value.RegisterSet = static_cast<ESM30RegisterSet>(ReadStream<uint8_t>(Stream));

	ReadStream(Stream, Value.RegisterStart);
	ReadStream(Stream, Value.ElementRegisterCount);
	ReadStream(Stream, Value.ElementCount);
	ReadStream(Stream, Value.Columns);
	ReadStream(Stream, Value.Rows);
	ReadStream(Stream, Value.Flags);

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30RsrcMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.Register);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30RsrcMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.Register);
	//???store sampler type or index for texture type validation on set?
	//???how to reference texture object in SM3.0 shader for it to be included in params list?
	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30SamplerMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream<uint8_t>(Stream, Value.Type);
	WriteStream(Stream, Value.RegisterStart);
	WriteStream(Stream, Value.RegisterCount);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30SamplerMeta& Value)
{
	ReadStream(Stream, Value.Name);

	uint8_t Type;
	ReadStream<uint8_t>(Stream, Type);
	Value.Type = static_cast<ESM30SamplerType>(Type);

	ReadStream(Stream, Value.RegisterStart);
	ReadStream(Stream, Value.RegisterCount);

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30ShaderMeta& Value)
{
	WriteStream<uint32_t>(Stream, Value.Buffers.size());
	for (const auto& Obj : Value.Buffers)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Structs.size());
	for (const auto& Obj : Value.Structs)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Consts.size());
	for (const auto& Obj : Value.Consts)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Resources.size());
	for (const auto& Obj : Value.Resources)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Samplers.size());
	for (const auto& Obj : Value.Samplers)
		Stream << Obj;

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30ShaderMeta& Value)
{
	uint32_t Count = 0;

	Value.Buffers.clear();
	Value.Structs.clear();
	Value.Consts.clear();
	Value.Resources.clear();
	Value.Samplers.clear();

	ReadStream<uint32_t>(Stream, Count);
	Value.Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30BufferMeta Obj;
		Stream >> Obj;
		Value.Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30StructMeta Obj;
		Stream >> Obj;
		Value.Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30ConstMeta Obj;
		Stream >> Obj;
		Value.Consts.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30RsrcMeta Obj;
		Stream >> Obj;
		Value.Resources.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30SamplerMeta Obj;
		Stream >> Obj;
		Value.Samplers.push_back(std::move(Obj));
	}

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CSM30EffectMeta& Value)
{
	WriteStream<uint32_t>(Stream, Value.Buffers.size());
	for (const auto& Obj : Value.Buffers)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Structs.size());
	for (const auto& Obj : Value.Structs)
		Stream << Obj;

	WriteStream<uint32_t>(Stream, Value.Consts.size());
	for (const auto& IDToMeta : Value.Consts)
	{
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Resources.size());
	for (const auto& IDToMeta : Value.Resources)
	{
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Samplers.size());
	for (const auto& IDToMeta : Value.Samplers)
	{
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CSM30EffectMeta& Value)
{
	uint32_t Count = 0;

	Value.Buffers.clear();
	Value.Structs.clear();
	Value.Consts.clear();
	Value.Resources.clear();
	Value.Samplers.clear();

	ReadStream<uint32_t>(Stream, Count);
	Value.Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30BufferMeta Obj;
		Stream >> Obj;
		Value.Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CSM30StructMeta Obj;
		Stream >> Obj;
		Value.Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CSM30ConstMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Consts.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CSM30RsrcMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Resources.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CSM30SamplerMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Samplers.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	return Stream;
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30BufferMeta& Value)
{
	uint32_t Total =
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint32_t) + // SlotIndex
		3 * sizeof(uint32_t); // Register range counters per type (float, int, bool)

	std::vector<const std::set<uint32_t>*> RegSets = { &Value.UsedFloat4, &Value.UsedInt4, &Value.UsedBool };
	for (const auto* pRegSet : RegSets)
	{
		std::vector<std::pair<uint32_t, uint32_t>> Ranges;
		BuildRegisterRanges(*pRegSet, Ranges);

		Total += sizeof(uint32_t) * 2 * static_cast<uint32_t>(Ranges.size()); // Range pairs
	}

	return Total;
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30ConstMetaBase& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		19; // Other data
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30StructMeta& Value)
{
	uint32_t Total = sizeof(uint8_t) + sizeof(uint32_t); // RegSet & Counter

	for (const auto& Member : Value.Members)
		Total += GetSerializedSize(Member);

	return Total;
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30ConstMeta& Value)
{
	return
		GetSerializedSize(static_cast<const CSM30ConstMetaBase&>(Value)) + // Base data
		sizeof(uint8_t) + // RegisterSet
		sizeof(uint32_t); // BufferIndex
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30RsrcMeta& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint32_t); // Register
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CSM30SamplerMeta& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint8_t) + // Type
		sizeof(uint32_t) + // RegisterStart
		sizeof(uint32_t); // RegisterCount
}
//---------------------------------------------------------------------

uint32_t GetSerializedSize(const CSM30ShaderMeta& Value)
{
	uint32_t Total = 5 * sizeof(uint32_t); // Counters

	for (const auto& Obj : Value.Buffers)
		Total += GetSerializedSize(Obj);

	for (const auto& Obj : Value.Structs)
		Total += GetSerializedSize(Obj);

	for (const auto& Obj : Value.Consts)
		Total += GetSerializedSize(Obj);

	for (const auto& Obj : Value.Resources)
		Total += GetSerializedSize(Obj);

	for (const auto& Obj : Value.Samplers)
		Total += GetSerializedSize(Obj);

	return Total;
}
//---------------------------------------------------------------------

uint32_t GetSerializedSize(const CSM30EffectMeta& Value)
{
	uint32_t Total = 5 * sizeof(uint32_t); // Counters

	for (const auto& Obj : Value.Buffers)
		Total += GetSerializedSize(Obj);

	for (const auto& Obj : Value.Structs)
		Total += GetSerializedSize(Obj);

	for (const auto& IDToMeta : Value.Consts)
		Total += GetSerializedSize(IDToMeta.second.second) + sizeof(uint8_t); // Object + shader type mask

	for (const auto& IDToMeta : Value.Resources)
		Total += GetSerializedSize(IDToMeta.second.second) + sizeof(uint8_t); // Object + shader type mask

	for (const auto& IDToMeta : Value.Samplers)
		Total += GetSerializedSize(IDToMeta.second.second) + sizeof(uint8_t); // Object + shader type mask

	return Total;
}
//---------------------------------------------------------------------

void MergeConstantBuffers(const CSM30BufferMeta& SrcBuffer, CSM30BufferMeta& TargetBuffer)
{
	TargetBuffer.UsedFloat4.insert(SrcBuffer.UsedFloat4.begin(), SrcBuffer.UsedFloat4.end());
	TargetBuffer.UsedInt4.insert(SrcBuffer.UsedInt4.begin(), SrcBuffer.UsedInt4.end());
	TargetBuffer.UsedBool.insert(SrcBuffer.UsedBool.begin(), SrcBuffer.UsedBool.end());
}
//---------------------------------------------------------------------

void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CSM30BufferMeta>& SrcBuffers, std::vector<CSM30BufferMeta>& TargetBuffers)
{
	if (BufferIndex == static_cast<uint32_t>(-1)) return;

	const auto& Buffer = SrcBuffers[BufferIndex];
	auto ItBuffer = std::find(TargetBuffers.begin(), TargetBuffers.end(), Buffer);
	if (ItBuffer != TargetBuffers.end())
	{
		// The same buffer found, reference it
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.begin(), ItBuffer));
		return;
	}

	// Buffer can differ due to registers of unused fields, try to merge if name is the same
	ItBuffer = std::find_if(TargetBuffers.begin(), TargetBuffers.end(), [&Buffer](const CSM30BufferMeta& TargetBuffer)
	{
		return Buffer.Name == TargetBuffer.Name && Buffer.SlotIndex == TargetBuffer.SlotIndex;
	});

	if (ItBuffer != TargetBuffers.end())
	{
		// Buffer with the same name and slot is found, merge them
		MergeConstantBuffers(Buffer, *ItBuffer);
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.begin(), ItBuffer));
	}
	else
	{
		// Copy new buffer to metadata
		TargetBuffers.push_back(Buffer);
		BufferIndex = static_cast<uint32_t>(TargetBuffers.size() - 1);
	}
}
//---------------------------------------------------------------------

bool CheckConstRegisterOverlapping(const CSM30ConstMeta& Param, const CSM30EffectMeta& Other)
{
	const auto& OtherRegs =
		(Param.RegisterSet == RS_Float4) ? Other.UsedFloat4 :
		(Param.RegisterSet == RS_Int4) ? Other.UsedInt4 :
		Other.UsedBool;

	// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
	const auto RegisterEnd = Param.RegisterStart + Param.ElementRegisterCount * Param.ElementCount;
	for (uint32_t r = Param.RegisterStart; r < RegisterEnd; ++r)
		if (OtherRegs.find(r) != OtherRegs.cend())
			return false;

	return true;
}
//---------------------------------------------------------------------

//???add logging?
bool CollectMaterialParams(CMaterialParams& Out, const CSM30EffectMeta& Meta)
{
	for (const auto& Const : Meta.Consts)
	{
		// Check type consistency
		if (Out.Samplers.find(Const.first) != Out.Samplers.cend()) return false;
		if (Out.Resources.find(Const.first) != Out.Resources.cend()) return false;

		const uint32_t RegisterCount = Const.second.second.ElementRegisterCount * Const.second.second.ElementCount;

		// FIXME: support structures (fill members of CMaterialConst)

		EShaderConstType Type;
		uint32_t ConstSizeInBytes = 0;
		switch (Const.second.second.RegisterSet)
		{
			case RS_Float4:
			{
				Type = EShaderConstType::Float;
				ConstSizeInBytes = 4 * sizeof(float) * RegisterCount;
				break;
			}
			case RS_Int4:
			{
				Type = EShaderConstType::Int;
				ConstSizeInBytes = 4 * sizeof(int) * RegisterCount;
				break;
			}
			case RS_Bool:
			{
				Type = EShaderConstType::Bool;
				ConstSizeInBytes = sizeof(bool) * RegisterCount;
				break;
			}
			default: return false;
		}

		auto It = Out.Consts.find(Const.first);
		if (It == Out.Consts.cend())
		{
			Out.Consts.emplace(Const.first, CMaterialConst{ Type, ConstSizeInBytes });
		}
		else
		{
			if (It->second.Type != Type) return false;

			if (It->second.SizeInBytes < ConstSizeInBytes)
				It->second.SizeInBytes = ConstSizeInBytes;
		}
	}

	for (const auto& Rsrc : Meta.Resources)
	{
		// Check type consistency
		if (Out.Samplers.find(Rsrc.first) != Out.Samplers.cend()) return false;
		if (Out.Consts.find(Rsrc.first) != Out.Consts.cend()) return false;

		Out.Resources.insert(Rsrc.first);
	}

	for (const auto& Sampler : Meta.Samplers)
	{
		// Check type consistency
		if (Out.Consts.find(Sampler.first) != Out.Consts.cend()) return false;
		if (Out.Resources.find(Sampler.first) != Out.Resources.cend()) return false;

		Out.Samplers.insert(Sampler.first);
	}

	return true;
}
//---------------------------------------------------------------------
