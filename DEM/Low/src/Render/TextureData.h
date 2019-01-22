#pragma once
#include <Resources/ResourceObject.h>
#include <Render/Texture.h> // for CTextureDesc

// Texture data is an engine resource that incapsulates all necessary data
// for a texture creation. Data is stored in a RAM and can be loaded from
// different formats or be generated procedurally. Each GPU can create its
// own CTexture object from this data.

// NB: for now pData must be either a mapped pointer to a stream or allocated by n_malloc / n_realloc.

namespace IO
{
	typedef Ptr<class CStream> PStream;
}

namespace Render
{

class CTextureData: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

public:

	// If stream is valid, pData is a mapped stream contents. Else pData must be n_free'd.
	IO::PStream		Stream;
	void*			pData = nullptr;

	CTextureDesc	Desc;
	bool			MipDataProvided = false;

	virtual ~CTextureData();

	virtual bool IsResourceValid() const { return pData && Desc.Width; }
};

typedef Ptr<CTextureData> PTextureData;

}
