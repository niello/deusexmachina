#pragma once
#ifndef __DEM_L1_BT_FILE_H__
#define __DEM_L1_BT_FILE_H__

#include <System/System.h>

// Binary terrain (.bt) file wrapper
// http://vterrain.org/Implementation/Formats/BT.html

namespace IO
{

class CBTFile
{
public:

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
			U32			Width;		// Columns
			U32			Height;		// Rows
			U16			DataSize;
			U16			IsFloat;
			union
			{
				I16		HorzUnits;	// BT 1.3
				I16		Projection;	// BT 1.1 and 1.2
			};
			I16			UTMZone;
			I16			Datum;
			double		LeftExtent;
			double		RightExtent;
			double		BottomExtent;
			double		TopExtent;

			// BT 1.2
			U16			IsProjExternal;

			// BT 1.3
			float		VerticalScale;
		};
#pragma pack(pop)
	};

	EVersion			Version;
	bool				SelfAllocated;
	float				MinHeight;
	float				MaxHeight;
	CHeaderUnion*		Header;
	union
	{
		float*			HeightsF;
		short*			HeightsS;
	};

	void InitVersion();

public:

	static const float NoDataF;
	static const short NoDataS;

	CBTFile(): Header(NULL), HeightsF(NULL), SelfAllocated(false), MinHeight(NoDataF), MaxHeight(NoDataF) {}
	CBTFile(const void* Data);
	~CBTFile() { if (SelfAllocated && Header) n_free(Header); }

	EVersion	GetFileVersion() const { return Version; }
	U64			GetFileSize() const { return sizeof(Header->_Header) + Header->Width * Header->Height * Header->DataSize; }
	UPTR		GetHeightCount() const { return Header->Width * Header->Height; }
	U64			GetHFDataSize() const { return Header->Width * Header->Height * Header->DataSize; }
	UPTR		GetWidth() const { return Header->Width; }
	UPTR		GetHeight() const { return Header->Height; }
	float		GetMinHeight();
	float		GetMaxHeight();
	double		GetLeftExtent() const { return Header->LeftExtent; }
	double		GetRightExtent() const { return Header->RightExtent; }
	double		GetBottomExtent() const { return Header->BottomExtent; }
	double		GetTopExtent() const { return Header->TopExtent; }
	bool		IsFloatData() const { return Header->IsFloat != 0; }
	float		GetVerticalScale() const { return (Header->VerticalScale > 0.f) ? Header->VerticalScale : 1.f; }
	float		GetHeightF(UPTR X, UPTR Z) const { return HeightsF[Z * Header->Width + X]; }
	short		GetHeightS(UPTR X, UPTR Z) const { return HeightsS[Z * Header->Width + X]; }
	float		GetHeight(UPTR X, UPTR Z) const;
	void		GetHeights(float* OutFloats, UPTR X = 0, UPTR Z = 0, UPTR W = 0, UPTR H = 0);

	const short*	GetHeightsS() { n_assert(!IsFloatData()); return HeightsS; }
};

}

#endif
