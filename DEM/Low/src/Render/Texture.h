#pragma once
#include <Core/Object.h>
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
	EMSAAQuality	MSAAQuality; //???move to render state & depth-stencil descs only?
};

class CTexture: public Core::CObject
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

}
