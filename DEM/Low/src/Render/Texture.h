#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// Texture GPU resource object that may be stored in VRAM and used for rendering

namespace Render
{

class CTexture: public Core::CObject
{
	RTTI_CLASS_DECL(Render::CTexture, Core::CObject);

protected:

	PTextureData	TextureData;
	Data::CFlags	Access;
	UPTR			RowPitch;
	UPTR			SlicePitch;
	//UPTR			LockCount;
	bool			HoldRAMBackingData = false;

	void InternalDestroy();

public:

	CTexture();
	virtual ~CTexture();

	virtual void		Destroy() { InternalDestroy(); }

	UPTR				GetPixelCount(bool IncludeSubsequentMips) const;
	const CTextureDesc&	GetDesc() const;
	Data::CFlags		GetAccess() const { return Access; }
	UPTR				GetDimensionCount() const;
	UPTR				GetRowPitch() const { return RowPitch; }
	UPTR				GetSlicePitch() const { return SlicePitch; }

	virtual void        SetDebugName(std::string_view Name) = 0;
};

typedef Ptr<CTexture> PTexture;

}
