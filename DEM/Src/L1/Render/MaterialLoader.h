#pragma once
#ifndef __DEM_L1_MATERIAL_LOADER_H__
#define __DEM_L1_MATERIAL_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads render material from DEM "prm" format

//namespace Render
//{
//	typedef Ptr<class CGPUDriver> PGPUDriver;
//}

namespace Resources
{

class CMaterialLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	virtual ~CMaterialLoader() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CMaterialLoader> PMaterialLoader;

}

#endif
