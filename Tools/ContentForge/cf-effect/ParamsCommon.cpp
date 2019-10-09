#include <RenderEnums.h>
#include <Utils.h>
#include <iostream>

uint32_t WriteFloatDefault(std::ostream& Stream, const Data::CData& DefaultValue)
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

uint32_t WriteIntDefault(std::ostream& Stream, const Data::CData& DefaultValue)
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

uint32_t WriteBoolDefault(std::ostream& Stream, const Data::CData& DefaultValue)
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

void SerializeSamplerState(std::ostream& Stream, const Data::CParams& SamplerDesc)
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
