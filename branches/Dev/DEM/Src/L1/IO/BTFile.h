#pragma once
#ifndef __DEM_L1_BT_FILE_H__
#define __DEM_L1_BT_FILE_H__

#include <Core/Core.h>

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
			DWORD		Width;		// Columns
			DWORD		Height;		// Rows
			short		DataSize;
			short		IsFloat;
			union
			{
				short	HorzUnits;	// BT 1.3
				short	Projection;	// BT 1.1 and 1.2
			};
			short		UTMZone;
			short		Datum;
			double		LeftExtent;
			double		RightExtent;
			double		BottomExtent;
			double		TopExtent;

			// BT 1.2
			short		IsProjExternal;

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
	//CBTFile(const CStream& File) {}
	//CBTFile(const CString& FileName) {}
	~CBTFile() { if (SelfAllocated && Header) n_free(Header); }

	EVersion	GetFileVersion() const { return Version; }
	DWORD		GetFileSize() const { return sizeof(Header->_Header) + Header->Width * Header->Height * Header->DataSize; }
	DWORD		GetHeightCount() const { return Header->Width * Header->Height; }
	DWORD		GetHFDataSize() const { return Header->Width * Header->Height * Header->DataSize; }
	DWORD		GetWidth() const { return Header->Width; }
	DWORD		GetHeight() const { return Header->Height; }
	float		GetMinHeight();
	float		GetMaxHeight();
	double		GetLeftExtent() const { return Header->LeftExtent; }
	double		GetRightExtent() const { return Header->RightExtent; }
	double		GetBottomExtent() const { return Header->BottomExtent; }
	double		GetTopExtent() const { return Header->TopExtent; }
	bool		IsFloatData() const { return Header->IsFloat != 0; }
	float		GetVerticalScale() const { return (Header->VerticalScale > 0.f) ? Header->VerticalScale : 1.f; }
	float		GetHeightF(DWORD X, DWORD Z) const { return HeightsF[Z * Header->Width + X]; }
	short		GetHeightS(DWORD X, DWORD Z) const { return HeightsS[Z * Header->Width + X]; }
	float		GetHeight(DWORD X, DWORD Z) const;
	void		GetHeights(float* OutFloats, DWORD X = 0, DWORD Z = 0, DWORD W = 0, DWORD H = 0);

	const short*	GetHeightsS() { n_assert(!IsFloatData()); return HeightsS; }
};

}

#endif
