#pragma once
#include "ShaderMetaCommon.h"
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <Logging.h>
#include <sstream>

static uint32_t WriteFloatDefault(std::ostream& Stream, const Data::CData& DefaultValue)
{
	if (DefaultValue.IsA<float>())
	{
		WriteStream(Stream, DefaultValue.GetValue<float>());
		return sizeof(float);
	}
	else if (DefaultValue.IsA<int>())
	{
		WriteStream(Stream, static_cast<float>(DefaultValue.GetValue<int>()));
		return sizeof(float);
	}
	else if (DefaultValue.IsA<vector4>())
	{
		WriteStream(Stream, DefaultValue.GetValue<vector4>());
		return sizeof(vector4);
	}
	else return 0;
}
//---------------------------------------------------------------------

static uint32_t WriteIntDefault(std::ostream& Stream, const Data::CData& DefaultValue)
{
	if (DefaultValue.IsA<int>())
	{
		WriteStream(Stream, DefaultValue.GetValue<int>());
		return sizeof(int);
	}
	else if (DefaultValue.IsA<float>())
	{
		WriteStream(Stream, static_cast<int>(DefaultValue.GetValue<float>()));
		return sizeof(int);
	}
	else if (DefaultValue.IsA<vector4>())
	{
		const auto& Vector = DefaultValue.GetValue<vector4>();
		WriteStream(Stream, static_cast<int>(Vector.x));
		WriteStream(Stream, static_cast<int>(Vector.y));
		WriteStream(Stream, static_cast<int>(Vector.z));
		WriteStream(Stream, static_cast<int>(Vector.w));
		return 4 * sizeof(int);
	}
	else return 0;
}
//---------------------------------------------------------------------

static uint32_t WriteBoolDefault(std::ostream& Stream, const Data::CData& DefaultValue)
{
	if (DefaultValue.IsA<bool>())
	{
		WriteStream(Stream, DefaultValue.GetValue<bool>());
		return sizeof(bool);
	}
	else if (DefaultValue.IsA<int>())
	{
		WriteStream<bool>(Stream, DefaultValue.GetValue<int>() != 0);
		return sizeof(bool);
	}
	else return 0;
}
//---------------------------------------------------------------------

static void SerializeSamplerState(std::ostream& Stream, const Data::CParams& SamplerDesc)
{
	// NB: default values are different for D3D9 and D3D11, but we use the same defaults for consistency

	std::string StrValue;
	vector4 ColorRGBA;

	if (TryGetParam(StrValue, SamplerDesc, "AddressU"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "AddressV"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "AddressW"))
		WriteStream<uint8_t>(Stream, StringToTexAddressMode(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexAddr_Wrap);

	if (TryGetParam(StrValue, SamplerDesc, "Filter"))
		WriteStream<uint8_t>(Stream, StringToTexFilter(StrValue));
	else
		WriteStream<uint8_t>(Stream, TexFilter_MinMagMip_Linear);

	if (!TryGetParam(ColorRGBA, SamplerDesc, "BorderColorRGBA"))
		ColorRGBA = { 0.f, 0.f, 0.f, 0.f };

	WriteStream(Stream, ColorRGBA);

	WriteStream(Stream, GetParam(SamplerDesc, "MipMapLODBias", 0.f));
	WriteStream(Stream, GetParam(SamplerDesc, "FinestMipMapLOD", 0.f));
	WriteStream(Stream, GetParam(SamplerDesc, "CoarsestMipMapLOD", std::numeric_limits<float>().max()));
	WriteStream<uint32_t>(Stream, GetParam<int>(SamplerDesc, "MaxAnisotropy", 1));

	if (TryGetParam(StrValue, SamplerDesc, "CmpFunc"))
		WriteStream<uint8_t>(Stream, StringToCmpFunc(StrValue));
	else
		WriteStream<uint8_t>(Stream, Cmp_Never);
}
//---------------------------------------------------------------------

bool WriteMaterialParams(std::ostream& Stream, const CMaterialParams& Table, const Data::CParams& Values, CThreadSafeLog& Log)
{
	const auto DefaultCountOffset = Stream.tellp();
	WriteStream<uint32_t>(Stream, 0);

	uint32_t DefaultCount = 0;
	std::ostringstream DefaultConstsStream(std::ios_base::binary);

	for (const auto& MaterialParam : Values)
	{
		const std::string ID = MaterialParam.first.CStr();
		auto ItConst = Table.Consts.find(ID);
		if (ItConst != Table.Consts.cend())
		{
			const uint32_t CurrDefaultOffset = static_cast<uint32_t>(DefaultConstsStream.tellp());

			if (MaterialParam.second.IsVoid()) continue;

			// TODO: support structures
			assert(ItConst->second.Type != EShaderConstType::Struct);

			uint32_t ValueSizeInBytes = 0;

			switch (ItConst->second.Type)
			{
				case EShaderConstType::Float:
				{
					if (ValueSizeInBytes = WriteFloatDefault(Stream, MaterialParam.second))
					{
						Log.LogWarning("Material param '" + ID + "' is a float, default value must be null, float, int or vector");
						return false;
					}
					break;
				}
				case EShaderConstType::Int:
				{
					if (ValueSizeInBytes = WriteIntDefault(Stream, MaterialParam.second))
					{
						Log.LogWarning("Material param '" + ID + "' is an integer, default value must be null, float, int or vector");
						return false;
					}
					break;
				}
				case EShaderConstType::Bool:
				{
					if (ValueSizeInBytes = WriteBoolDefault(Stream, MaterialParam.second))
					{
						Log.LogWarning("Material param '" + ID + "' is a bool, default value must be null, bool or int");
						return false;
					}
					break;
				}
				default:
				{
					Log.LogWarning("Material param '" + ID + "' is a constant of unsupported type or register set, default value is skipped");
					return false;
				}
			}

			WriteStream(Stream, ID);
			WriteStream(Stream, CurrDefaultOffset);
			++DefaultCount;
		}
		else if (Table.Resources.find(ID) != Table.Resources.cend())
		{
			if (MaterialParam.second.IsA<CStrID>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<CStrID>().ToString());
				++DefaultCount;
			}
			else if (MaterialParam.second.IsA<std::string>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<std::string>());
				++DefaultCount;
			}
			else if (!MaterialParam.second.IsVoid())
			{
				Log.LogWarning("Unsupported default type for resource '" + ID + "', must be string or string ID");
			}
		}
		else if (Table.Samplers.find(ID) != Table.Samplers.cend())
		{
			if (MaterialParam.second.IsA<Data::CParams>())
			{
				WriteStream(Stream, ID);
				SerializeSamplerState(Stream, MaterialParam.second.GetValue<Data::CParams>());
				++DefaultCount;
			}
			else if (!MaterialParam.second.IsVoid())
			{
				Log.LogWarning("Unsupported default type for sampler '" + ID + "', must be section with sampler settings");
			}
		}
		else
		{
			Log.LogWarning("Default for unknown material parameter '" + ID + "' is skipped");
		}
	}

	// Write actual default value count

	const auto EndOffset = Stream.tellp();
	Stream.seekp(DefaultCountOffset, std::ios_base::beg);
	WriteStream<uint32_t>(Stream, DefaultCount);
	Stream.seekp(EndOffset, std::ios_base::beg);

	// Write the buffer with constant defaults

	const std::string DefaultConstsBuffer = DefaultConstsStream.str();
	WriteStream(Stream, static_cast<uint32_t>(DefaultConstsBuffer.size()));
	if (!DefaultConstsBuffer.empty())
		Stream.write(DefaultConstsBuffer.c_str(), DefaultConstsBuffer.size());

	return true;
}
//---------------------------------------------------------------------
