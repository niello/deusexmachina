#include "USMShaderMeta.h"
#include <Utils.h>

bool CUSMShaderMeta::Save(std::ostream& Stream) const
{
	WriteStream<uint64_t>(Stream, RequiresFlags);

	WriteStream<uint32_t>(Stream, Buffers.size());
	for (size_t i = 0; i < Buffers.size(); ++i)
	{
		const CUSMBufferMeta& Obj = Buffers[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.Register);
		WriteStream(Stream, Obj.Size);
		//WriteStream(Stream, Obj.ElementCount);
	}

	WriteStream<uint32_t>(Stream, Structs.size());
	for (size_t i = 0; i < Structs.size(); ++i)
	{
		const CUSMStructMeta& Obj = Structs[i];

		WriteStream<uint32_t>(Stream, Obj.Members.size());
		for (size_t j = 0; j < Obj.Members.size(); ++j)
		{
			const CUSMStructMemberMeta& Member = Obj.Members[j];
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
	}

	WriteStream<uint32_t>(Stream, Consts.size());
	for (size_t i = 0; i < Consts.size(); ++i)
	{
		const CUSMConstMeta& Obj = Consts[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.BufferIndex);
		WriteStream(Stream, Obj.StructIndex);
		WriteStream<uint8_t>(Stream, Obj.Type);
		WriteStream(Stream, Obj.Offset);
		WriteStream(Stream, Obj.ElementSize);
		WriteStream(Stream, Obj.ElementCount);
		WriteStream(Stream, Obj.Columns);
		WriteStream(Stream, Obj.Rows);
		WriteStream(Stream, Obj.Flags);
	}

	WriteStream<uint32_t>(Stream, Resources.size());
	for (size_t i = 0; i < Resources.size(); ++i)
	{
		const CUSMRsrcMeta& Obj = Resources[i];
		WriteStream(Stream, Obj.Name);
		WriteStream<uint8_t>(Stream, Obj.Type);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.RegisterCount);
	}

	WriteStream<uint32_t>(Stream, Samplers.size());
	for (size_t i = 0; i < Samplers.size(); ++i)
	{
		const CUSMSamplerMeta& Obj = Samplers[i];
		WriteStream(Stream, Obj.Name);
		WriteStream(Stream, Obj.RegisterStart);
		WriteStream(Stream, Obj.RegisterCount);
	}

	return true;
}
//---------------------------------------------------------------------

bool CUSMShaderMeta::Load(std::istream& Stream)
{
	Buffers.clear();
	Consts.clear();
	Resources.clear();
	Samplers.clear();

	ReadStream<uint64_t>(Stream, RequiresFlags);

	uint32_t Count;

	ReadStream<uint32_t>(Stream, Count);
	Buffers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CUSMBufferMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.Register);
		ReadStream(Stream, Obj.Size);
		///ReadStream(Obj.ElementCount);
		Buffers.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Structs.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMStructMeta Obj;

		uint32_t MemberCount;
		ReadStream<uint32_t>(Stream, MemberCount);
		Obj.Members.reserve(MemberCount);
		for (uint32_t j = 0; j < MemberCount; ++j)
		{
			CUSMStructMemberMeta Member;
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

			Obj.Members.push_back(std::move(Member));
		}

		Structs.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Consts.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMConstMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.BufferIndex);
		ReadStream(Stream, Obj.StructIndex);

		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Obj.Type = (EUSMConstType)Type;

		ReadStream(Stream, Obj.Offset);
		ReadStream(Stream, Obj.ElementSize);
		ReadStream(Stream, Obj.ElementCount);
		ReadStream(Stream, Obj.Columns);
		ReadStream(Stream, Obj.Rows);
		ReadStream(Stream, Obj.Flags);

		Consts.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Resources.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMRsrcMeta Obj;
		ReadStream(Stream, Obj.Name);

		uint8_t Type;
		ReadStream<uint8_t>(Stream, Type);
		Obj.Type = (EUSMResourceType)Type;

		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.RegisterCount);

		Resources.push_back(std::move(Obj));
	}

	ReadStream<uint32_t>(Stream, Count);
	Samplers.reserve(Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		CUSMSamplerMeta Obj;
		ReadStream(Stream, Obj.Name);
		ReadStream(Stream, Obj.RegisterStart);
		ReadStream(Stream, Obj.RegisterCount);
		Samplers.push_back(std::move(Obj));
	}

	return true;
}
//---------------------------------------------------------------------
