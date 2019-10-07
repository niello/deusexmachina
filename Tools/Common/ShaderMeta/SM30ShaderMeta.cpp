#include "SM30ShaderMeta.h"
#include <Utils.h>

static void WriteRegisterRanges(const std::set<uint32_t>& UsedRegs, std::ostream& Stream)
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

std::ostream& operator <<(std::ostream& Stream, const CSM30BufferMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.SlotIndex);

	WriteRegisterRanges(Value.UsedFloat4, Stream);
	WriteRegisterRanges(Value.UsedInt4, Stream);
	WriteRegisterRanges(Value.UsedBool, Stream);

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
	WriteStream<uint32_t>(Stream, Value.Members.size());
	for (const auto& Member : Value.Members)
	{
		WriteStream(Stream, Member.Name);
		WriteStream(Stream, Member.StructIndex);
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

	uint8_t RegSet;
	ReadStream<uint8_t>(Stream, RegSet);
	Value.RegisterSet = (ESM30RegisterSet)RegSet;

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
