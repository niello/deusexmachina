#pragma once
#include <Core/Object.h>
#include <Render/Texture.h> // for CTextureDesc

// Texture data is an engine resource that incapsulates all necessary data
// for a texture creation. Data is stored in a RAM and can be loaded from
// different formats or be generated procedurally. Each GPU can create its
// own CTexture object from this data.

namespace Data
{
	typedef std::unique_ptr<class IBuffer> PBuffer;
}

namespace Render
{

class CTextureData: public ::Core::CObject
{
	RTTI_CLASS_DECL(Render::CTextureData, ::Core::CObject);

private:

	UPTR          BufferUseCounter = 0;

public:

	Data::PBuffer Data;
	CTextureDesc  Desc;
	bool          MipDataProvided = false; // Does Data provide all mips or only a top-level data

	CTextureData();
	virtual ~CTextureData();

	// Controls RAM texture data lifetime. Some GPU resources may want to keep this data in RAM.
	bool			UseBuffer();
	void			ReleaseBuffer();
};

typedef Ptr<CTextureData> PTextureData;

}
