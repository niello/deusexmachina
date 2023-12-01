#include "USMShaderMeta.h"
#include <Utils.h>

std::ostream& operator <<(std::ostream& Stream, const CUSMBufferMeta& Value)
{
	WriteStream(Stream, Value.Name);
	WriteStream(Stream, Value.Register);
	WriteStream(Stream, Value.Size);
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
	for (const auto& Member : Value.Members)
	{
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
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMConstMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Consts.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMRsrcMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Resources.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	ReadStream<uint32_t>(Stream, Count);
	for (uint32_t i = 0; i < Count; ++i)
	{
		auto ShaderTypeMask = ReadStream<uint8_t>(Stream);
		CUSMSamplerMeta Param;
		Stream >> Param;
		std::string ParamName = Param.Name;
		Value.Samplers.emplace(std::move(ParamName), std::make_pair(ShaderTypeMask, std::move(Param)));
	}

	return Stream;
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMBufferMeta& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint32_t) + // Register
		sizeof(uint32_t); // Size
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMConstMetaBase& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		20; // Other data
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMStructMeta& Value)
{
	uint32_t Total = sizeof(uint32_t); // Counter

	for (const auto& Member : Value.Members)
		Total += GetSerializedSize(Member);

	return Total;
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMConstMeta& Value)
{
	return
		GetSerializedSize(static_cast<const CUSMConstMetaBase&>(Value)) + // Base data
		sizeof(uint32_t); // BufferIndex
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMRsrcMeta& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint8_t) + // Type
		sizeof(uint32_t) + // RegisterStart
		sizeof(uint32_t); // RegisterCount
}
//---------------------------------------------------------------------

static uint32_t GetSerializedSize(const CUSMSamplerMeta& Value)
{
	return
		sizeof(uint16_t) + static_cast<uint32_t>(Value.Name.size()) + // Name
		sizeof(uint32_t) + // RegisterStart
		sizeof(uint32_t); // RegisterCount
}
//---------------------------------------------------------------------

uint32_t GetSerializedSize(const CUSMShaderMeta& Value)
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

uint32_t GetSerializedSize(const CUSMEffectMeta& Value)
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

void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<CUSMBufferMeta>& SrcBuffers, std::vector<CUSMBufferMeta>& TargetBuffers)
{
	if (BufferIndex == static_cast<uint32_t>(-1)) return;

	const auto& Buffer = SrcBuffers[BufferIndex];
	auto ItBuffer = std::find(TargetBuffers.cbegin(), TargetBuffers.cend(), Buffer);
	if (ItBuffer != TargetBuffers.cend())
	{
		// The same buffer found, reference it
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.cbegin(), ItBuffer));
	}
	else
	{
		// Copy new buffer to metadata
		TargetBuffers.push_back(Buffer);
		BufferIndex = static_cast<uint32_t>(TargetBuffers.size() - 1);
	}
}
//---------------------------------------------------------------------

//???add logging?
bool CollectMaterialParams(CMaterialParams& Out, const CUSMEffectMeta& Meta)
{
	for (const auto& Const : Meta.Consts)
	{
		// Check type consistency
		if (Out.Samplers.find(Const.first) != Out.Samplers.cend()) return false;
		if (Out.Resources.find(Const.first) != Out.Resources.cend()) return false;

		// FIXME: support structures (fill members of CMaterialConst)

		EShaderConstType Type;
		switch (Const.second.second.Type)
		{
			case USMConst_Float: Type = EShaderConstType::Float; break;
			case USMConst_Int: Type = EShaderConstType::Int; break;
			case USMConst_Bool: Type = EShaderConstType::Bool; break;
			case USMConst_Struct: Type = EShaderConstType::Struct; break;
			default: return false;
		}

		const uint32_t ConstSizeInBytes = Const.second.second.ElementSize * std::max<uint32_t>(1, Const.second.second.ElementCount);

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
