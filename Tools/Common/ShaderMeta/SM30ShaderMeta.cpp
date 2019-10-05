#include "SM30ShaderMeta.h"
#include <Utils.h>

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
