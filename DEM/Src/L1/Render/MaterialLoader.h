#pragma once
#ifndef __DEM_L1_MATERIAL_LOADER_H__
#define __DEM_L1_MATERIAL_LOADER_H__

#include <Resources/ResourceLoader.h>
#include <Data/Dictionary.h>

// Loads render material from DEM "prm" format

namespace IO
{
	class CBinaryReader;
}

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
	typedef Ptr<class CTexture> PTexture;
	typedef Ptr<class CSampler> PSampler;
}

namespace Resources
{

class CMaterialLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver GPU;

	virtual ~CMaterialLoader() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CMaterialLoader> PMaterialLoader;

}

#endif
