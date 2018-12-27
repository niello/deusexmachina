#pragma once
#ifndef __DEM_L1_RENDER_TEXTURE_H__
#define __DEM_L1_RENDER_TEXTURE_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// Texture resource object that may be stored in VRAM and used for rendering

namespace Render
{

struct CTextureDesc
{
	ETextureType	Type;
	UPTR			Width;
	UPTR			Height;
	UPTR			Depth;
	UPTR			MipLevels;
	UPTR			ArraySize;
	EPixelFormat	Format;
	EMSAAQuality	MSAAQuality;
};

class CTexture: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	CTextureDesc	Desc;
	Data::CFlags	Access;
	UPTR			RowPitch;
	UPTR			SlicePitch;

	//UPTR			LockCount;

public:

	//virtual ~CTexture() {}

	virtual void		Destroy() = 0;

	UPTR				GetPixelCount(bool IncludeSubsequentMips) const;
	const CTextureDesc&	GetDesc() const { return Desc; }
	Data::CFlags		GetAccess() const { return Access; }
	UPTR				GetDimensionCount() const;
	UPTR				GetRowPitch() const { return RowPitch; }
	UPTR				GetSlicePitch() const { return SlicePitch; }
};

typedef Ptr<CTexture> PTexture;

inline UPTR CTexture::GetDimensionCount() const
{
	switch (Desc.Type)
	{
		case Texture_1D:	return 1;
		case Texture_2D:
		case Texture_Cube:	return 2;
		case Texture_3D:	return 3;
		default:			return 0;
	};
}
//---------------------------------------------------------------------

}

//DECLARE_TYPE(Render::PTexture, 14)
//#define TTexture DATA_TYPE(Render::PTexture)

#endif
