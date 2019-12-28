#pragma once
#include <cstdint>
#include <limits>

// Binary terrain (.bt) file wrapper
// http://vterrain.org/Implementation/Formats/BT.html

class CBTFile final
{
public:

	static constexpr float NoDataF = std::numeric_limits<float>::lowest();
	static constexpr short NoDataS = std::numeric_limits<short>::lowest();
	static constexpr uint16_t NoDataUS = static_cast<uint16_t>(static_cast<int>(NoDataS) + 32768);

	enum EVersion
	{
		BT_V_1_1 = 1,
		BT_V_1_2 = 2,
		BT_V_1_3 = 3,
		BT_INVALID
	};

	// HorzUnits field - horizontal units. Vertical units are always meters (above the sea level).
	enum
	{
		BT_HU_DEGREES				= 0,
		BT_HU_METERS				= 1,
		BT_HU_FEET_INTERNATIONAL	= 2,
		BT_HU_FEET_US				= 3
	};

protected:

	union CHeaderUnion
	{
		char			_Header[256];

#pragma pack(push, 1)
		struct
		{
			char		ID[10];
			uint32_t	Width;		// Columns
			uint32_t	Height;		// Rows
			uint16_t	DataSize;
			uint16_t	IsFloat;
			union
			{
				int16_t		HorzUnits;	// BT 1.3
				int16_t		Projection;	// BT 1.1 and 1.2
			};
			int16_t		UTMZone;
			int16_t		Datum;
			double		LeftExtent;
			double		RightExtent;
			double		BottomExtent;
			double		TopExtent;

			// BT 1.2
			uint16_t	IsProjExternal;

			// BT 1.3
			float		VerticalScale;
		};
#pragma pack(pop)
	};

	EVersion			Version;
	float				MinHeight;
	float				MaxHeight;
	const CHeaderUnion*	Header = nullptr;
	union
	{
		const float*	HeightsF = nullptr;
		const short*	HeightsS;
	};

	void InitVersion();

public:

	CBTFile(const void* Data);

	EVersion	GetFileVersion() const { return Version; }
	uint64_t	GetFileSize() const { return sizeof(Header->_Header) + Header->Width * Header->Height * Header->DataSize; }
	size_t		GetHeightCount() const { return Header->Width * Header->Height; }
	uint64_t	GetHFDataSize() const { return Header->Width * Header->Height * Header->DataSize; }
	size_t		GetWidth() const { return Header->Width; }
	size_t		GetHeight() const { return Header->Height; }
	float		GetMinHeight();
	float		GetMaxHeight();
	double		GetLeftExtent() const { return Header->LeftExtent; }
	double		GetRightExtent() const { return Header->RightExtent; }
	double		GetBottomExtent() const { return Header->BottomExtent; }
	double		GetTopExtent() const { return Header->TopExtent; }
	bool		IsFloatData() const { return Header->IsFloat != 0; }
	float		GetVerticalScale() const { return (Header->VerticalScale > 0.f) ? Header->VerticalScale : 1.f; }
	float		GetHeightF(size_t X, size_t Z) const { return HeightsF[Z * Header->Width + X]; }
	short		GetHeightS(size_t X, size_t Z) const { return HeightsS[Z * Header->Width + X]; }
	float		GetHeight(size_t X, size_t Z) const;
	void		GetHeights(float* OutFloats, size_t X = 0, size_t Z = 0, size_t W = 0, size_t H = 0);

	const short* GetHeightsS() { return IsFloatData() ? nullptr : HeightsS; }
};
