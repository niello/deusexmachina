#pragma once
#include <Resources/ResourceObject.h>
#include <Render/Texture.h> // for CTextureDesc

// Texture data is an engine resource that incapsulates all necessary data
// for a texture creation. Data is stored in a RAM and can be loaded from
// different formats or be generated procedurally. Each GPU can create its
// own CTexture object from this data.

namespace Data
{
	typedef std::unique_ptr<class IRAMData> PRAMData;
}

namespace Render
{

class CTextureData: public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

private:

	UPTR			RAMDataUseCounter = 0;

public:

	Data::PRAMData	Data;
	CTextureDesc	Desc;
	bool			MipDataProvided = false; // Does Data provide all mips or only a top-level data

	CTextureData();
	virtual ~CTextureData();

	virtual bool	IsResourceValid() const { return Data && Desc.Width; }

	// Controls RAM texture data lifetime. Some GPU resources may want to keep this data in RAM.
	bool			UseRAMData();
	void			ReleaseRAMData();
};

typedef Ptr<CTextureData> PTextureData;

}
