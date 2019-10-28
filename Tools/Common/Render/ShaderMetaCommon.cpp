#pragma once
#include "ShaderMetaCommon.h"
#include <Render/RenderEnums.h>
#include <Utils.h>
#include <Logging.h>
#include <sstream>

static uint32_t WriteFloatValue(std::ostream& Stream, const Data::CData& Value, uint32_t MaxBytes)
{
	if (Value.IsA<float>())
	{
		assert(MaxBytes >= sizeof(float));
		WriteStream(Stream, Value.GetValue<float>());
		return sizeof(float);
	}
	else if (Value.IsA<int>())
	{
		assert(MaxBytes >= sizeof(float));
		WriteStream(Stream, static_cast<float>(Value.GetValue<int>()));
		return sizeof(float);
	}
	else if (Value.IsA<vector4>())
	{
		// TODO: implement value truncation?
		assert(MaxBytes >= sizeof(vector4));
		WriteStream(Stream, Value.GetValue<vector4>());
		return sizeof(vector4);
	}
	else return 0;
}
//---------------------------------------------------------------------

static uint32_t WriteIntValue(std::ostream& Stream, const Data::CData& Value, uint32_t MaxBytes)
{
	if (Value.IsA<int>())
	{
		assert(MaxBytes >= sizeof(int));
		WriteStream(Stream, Value.GetValue<int>());
		return sizeof(int);
	}
	else if (Value.IsA<float>())
	{
		assert(MaxBytes >= sizeof(int));
		WriteStream(Stream, static_cast<int>(Value.GetValue<float>()));
		return sizeof(int);
	}
	else if (Value.IsA<vector4>())
	{
		// TODO: implement value truncation?
		assert(MaxBytes >= 4 * sizeof(int));
		const auto& Vector = Value.GetValue<vector4>();
		WriteStream(Stream, static_cast<int>(Vector.x));
		WriteStream(Stream, static_cast<int>(Vector.y));
		WriteStream(Stream, static_cast<int>(Vector.z));
		WriteStream(Stream, static_cast<int>(Vector.w));
		return 4 * sizeof(int);
	}
	else return 0;
}
//---------------------------------------------------------------------

static uint32_t WriteBoolValue(std::ostream& Stream, const Data::CData& Value, uint32_t MaxBytes)
{
	assert(MaxBytes >= 1);
	if (Value.IsA<bool>())
	{
		WriteStream(Stream, Value.GetValue<bool>());
		return sizeof(bool);
	}
	else if (Value.IsA<int>())
	{
		WriteStream<bool>(Stream, Value.GetValue<int>() != 0);
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
	const auto CountOffset = Stream.tellp();
	WriteStream<uint32_t>(Stream, 0);

	uint32_t Count = 0;
	std::ostringstream ConstsStream(std::ios_base::binary);

	for (const auto& MaterialParam : Values)
	{
		if (MaterialParam.second.IsVoid()) continue;

		const std::string ID = MaterialParam.first.CStr();

		Log.LogInfo("Writing value for param '" + ID + "'");

		auto ItConst = Table.Consts.find(ID);
		if (ItConst != Table.Consts.cend())
		{
			const uint32_t ConstSizeInBytes = ItConst->second.SizeInBytes;
			if (!ConstSizeInBytes)
			{
				Log.LogWarning("Material param '" + ID + "' has zero length, value is skipped");
				continue;
			}

			const uint32_t CurrOffset = static_cast<uint32_t>(ConstsStream.tellp());
			uint32_t ValueSizeInBytes = 0;

			switch (ItConst->second.Type)
			{
				case EShaderConstType::Float:
				{
					if (ValueSizeInBytes = WriteFloatValue(Stream, MaterialParam.second, ConstSizeInBytes))
					{
						Log.LogWarning("Material param '" + ID + "' is a float, value must be null, float, int or vector");
						return false;
					}
					break;
				}
				case EShaderConstType::Int:
				{
					if (ValueSizeInBytes = WriteIntValue(Stream, MaterialParam.second, ConstSizeInBytes))
					{
						Log.LogWarning("Material param '" + ID + "' is an integer, value must be null, float, int or vector");
						return false;
					}
					break;
				}
				case EShaderConstType::Bool:
				{
					if (ValueSizeInBytes = WriteBoolValue(Stream, MaterialParam.second, ConstSizeInBytes))
					{
						Log.LogWarning("Material param '" + ID + "' is a bool, value must be null, bool or int");
						return false;
					}
					break;
				}
				case EShaderConstType::Struct:
				{
					// TODO: support structures (CParams with recursion)
					Log.LogWarning("Material param '" + ID + "' is a structure. Structure values are not supported yet.");
					continue;
				}
				default:
				{
					Log.LogWarning("Material param '" + ID + "' is a constant of unsupported type or register set, value is skipped");
					continue;
				}
			}

			WriteStream(Stream, ID);
			WriteStream(Stream, CurrOffset);
			WriteStream(Stream, ValueSizeInBytes);
			++Count;
		}
		else if (Table.Resources.find(ID) != Table.Resources.cend())
		{
			if (MaterialParam.second.IsA<CStrID>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<CStrID>().ToString());
				++Count;
			}
			else if (MaterialParam.second.IsA<std::string>())
			{
				WriteStream(Stream, ID);
				WriteStream(Stream, MaterialParam.second.GetValue<std::string>());
				++Count;
			}
			else
			{
				Log.LogWarning("Unsupported type for resource '" + ID + "', must be string or string ID");
			}
		}
		else if (Table.Samplers.find(ID) != Table.Samplers.cend())
		{
			if (MaterialParam.second.IsA<Data::CParams>())
			{
				WriteStream(Stream, ID);
				SerializeSamplerState(Stream, MaterialParam.second.GetValue<Data::CParams>());
				++Count;
			}
			else
			{
				Log.LogWarning("Unsupported type for sampler '" + ID + "', must be section with sampler settings");
			}
		}
		else
		{
			Log.LogWarning("Value for unknown material parameter '" + ID + "' is skipped");
		}
	}

	// Write actual value count

	const auto EndOffset = Stream.tellp();
	Stream.seekp(CountOffset, std::ios_base::beg);
	WriteStream<uint32_t>(Stream, Count);
	Stream.seekp(EndOffset, std::ios_base::beg);

	// Write the buffer with constant values

	const std::string ConstsBuffer = ConstsStream.str();
	WriteStream(Stream, static_cast<uint32_t>(ConstsBuffer.size()));
	if (!ConstsBuffer.empty())
		Stream.write(ConstsBuffer.c_str(), ConstsBuffer.size());

	return true;
}
//---------------------------------------------------------------------
