#include "USMShaderMeta.h"
#include <Utils.h>

std::ostream& operator <<(std::ostream& Stream, const CUSMBufferMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.Register);
	WriteStream(Stream, Value.Size);
	//WriteStream(Stream, Value.ElementCount);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMBufferMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.Register);
	ReadStream(Stream, Value.Size);
	///ReadStream(Value.ElementCount);
	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMStructMeta& Value)
{
	WriteStream<uint32_t>(Stream, Value.Members.size());
	for (size_t j = 0; j < Value.Members.size(); ++j)
	{
		const CUSMConstMetaBase& Member = Value.Members[j];
		WriteStream(Stream, Member.Name);
		WriteStream(Stream, Member.StructIndex);
		WriteStream<uint8_t>(Stream, Member.Type);
		WriteStream(Stream, Member.Offset);
		WriteStream(Stream, Member.ElementSize);
		WriteStream(Stream, Member.ElementCount);
		WriteStream(Stream, Member.Columns);
		WriteStream(Stream, Member.Rows);
		WriteStream(Stream, Member.Flags);
	}

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMStructMeta& Value)
{
	uint32_t MemberCount;
	ReadStream<uint32_t>(Stream, MemberCount);
	Value.Members.reserve(MemberCount);
	for (uint32_t j = 0; j < MemberCount; ++j)
	{
		CUSMConstMetaBase Member;
		ReadStream(Stream, Member.Name);
		ReadStream(Stream, Member.StructIndex);

		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Member.Type = (EUSMConstType)Type;

		ReadStream(Stream, Member.Offset);
		ReadStream(Stream, Member.ElementSize);
		ReadStream(Stream, Member.ElementCount);
		ReadStream(Stream, Member.Columns);
		ReadStream(Stream, Member.Rows);
		ReadStream(Stream, Member.Flags);

		Value.Members.push_back(std::move(Member));
	}

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMConstMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.BufferIndex);
	WriteStream(Stream, Value.StructIndex);
	WriteStream<uint8_t>(Stream, Value.Type);
	WriteStream(Stream, Value.Offset);
	WriteStream(Stream, Value.ElementSize);
	WriteStream(Stream, Value.ElementCount);
	WriteStream(Stream, Value.Columns);
	WriteStream(Stream, Value.Rows);
	WriteStream(Stream, Value.Flags);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMConstMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.BufferIndex);
	ReadStream(Stream, Value.StructIndex);

	uint8_t Type;
	ReadStream<uint8_t>(Stream, Type);
	Value.Type = (EUSMConstType)Type;

	ReadStream(Stream, Value.Offset);
	ReadStream(Stream, Value.ElementSize);
	ReadStream(Stream, Value.ElementCount);
	ReadStream(Stream, Value.Columns);
	ReadStream(Stream, Value.Rows);
	ReadStream(Stream, Value.Flags);

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMRsrcMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream<uint8_t>(Stream, Value.Type);
	WriteStream(Stream, Value.RegisterStart);
	WriteStream(Stream, Value.RegisterCount);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMRsrcMeta& Value)
{
	ReadStream(Stream, Value.Name);

	uint8_t Type;
	ReadStream<uint8_t>(Stream, Type);
	Value.Type = (EUSMResourceType)Type;

	ReadStream(Stream, Value.RegisterStart);
	ReadStream(Stream, Value.RegisterCount);

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMSamplerMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.RegisterStart);
	WriteStream(Stream, Value.RegisterCount);
	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMSamplerMeta& Value)
{
	ReadStream(Stream, Value.Name);
	ReadStream(Stream, Value.RegisterStart);
	ReadStream(Stream, Value.RegisterCount);
	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMShaderMeta& Value)
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

std::istream& operator >>(std::istream& Stream, CUSMShaderMeta& Value)
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
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CUSMBufferMeta Obj;
		Stream >> Obj;
		Value.Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMStructMeta Obj;
		Stream >> Obj;
		Value.Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMConstMeta Obj;
		Stream >> Obj;
		Value.Consts.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMRsrcMeta Obj;
		Stream >> Obj;
		Value.Resources.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMSamplerMeta Obj;
		Stream >> Obj;
		Value.Samplers.push_back(std::move(Obj));
	}

	return Stream;
}
//---------------------------------------------------------------------

std::ostream& operator <<(std::ostream& Stream, const CUSMEffectMeta& Value)
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
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Resources.size());
	for (const auto& IDToMeta : Value.Resources)
	{
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	WriteStream<uint32_t>(Stream, Value.Samplers.size());
	for (const auto& IDToMeta : Value.Samplers)
	{
		WriteStream(Stream, IDToMeta.first);
		WriteStream(Stream, IDToMeta.second.first);
		Stream << IDToMeta.second.second;
	}

	return Stream;
}
//---------------------------------------------------------------------

std::istream& operator >>(std::istream& Stream, CUSMEffectMeta& Value)
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
		CUSMBufferMeta Obj;
		Stream >> Obj;
		Value.Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Value.Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMStructMeta Obj;
		Stream >> Obj;
		Value.Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ParamName = ReadStream<std::string>(Stream);
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMConstMeta Param;
		Stream >> Param;
		Value.Consts.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ParamName = ReadStream<std::string>(Stream);
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMRsrcMeta Param;
		Stream >> Param;
		Value.Resources.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ParamName = ReadStream<std::string>(Stream);
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMSamplerMeta Param;
		Stream >> Param;
		Value.Samplers.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	return Stream;
}
//---------------------------------------------------------------------
