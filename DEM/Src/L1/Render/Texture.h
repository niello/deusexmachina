#pragma once
#ifndef __DEM_L1_RENDER_TEXTURE_H__
#define __DEM_L1_RENDER_TEXTURE_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>

// Texture resource object that may be stored in VRAM and used for rendering

namespace Render
{

struct CTextureDesc
{
	ETextureType	Type;
	DWORD			Width;
	DWORD			Height;
	DWORD			Depth;
	DWORD			MipLevels;
	DWORD			ArraySize;
	EPixelFormat	Format;
	EMSAAQuality	MSAAQuality;
};

class CTexture: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

public:

	struct CMapInfo
	{        
		void*	pData;
		int		RowPitch;
		int		DepthPitch;

		CMapInfo(): pData(NULL), RowPitch(0), DepthPitch(0) {}
	};

protected:

	CTextureDesc	Desc;
	Data::CFlags	Access;

	//DWORD			LockCount;

public:

	//virtual ~CTexture() {}

	virtual void		Destroy() = 0;

	DWORD				GetPixelCount(bool IncludeSubsequentMips) const;
	const CTextureDesc&	GetDesc() const { return Desc; }
	Data::CFlags		GetAccess() const { return Access; }
	DWORD				GetDimensionCount() const;
};

typedef Ptr<CTexture> PTexture;

inline DWORD CTexture::GetDimensionCount() const
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
