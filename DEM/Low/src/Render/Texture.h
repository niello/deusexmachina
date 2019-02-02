#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// Texture GPU resource object that may be stored in VRAM and used for rendering

namespace Render
{

class CTexture: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	PTextureData	TextureData;
	Data::CFlags	Access;
	UPTR			RowPitch;
	UPTR			SlicePitch;

	//UPTR			LockCount;

public:

	CTexture();
	virtual ~CTexture();

	virtual void		Destroy() = 0;

	UPTR				GetPixelCount(bool IncludeSubsequentMips) const;
	const CTextureDesc&	GetDesc() const;
	Data::CFlags		GetAccess() const { return Access; }
	UPTR				GetDimensionCount() const;
	UPTR				GetRowPitch() const { return RowPitch; }
	UPTR				GetSlicePitch() const { return SlicePitch; }
};

typedef Ptr<CTexture> PTexture;

}
