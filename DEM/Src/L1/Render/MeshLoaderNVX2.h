#pragma once
#ifndef __DEM_L1_MESH_LOADER_NVX2_H__
#define __DEM_L1_MESH_LOADER_NVX2_H__

#include <Render/MeshLoader.h>

// Loads CMesh from The Nebula Device 2 "nvx2" format

namespace Resources
{

class CMeshLoaderNVX2: public CMeshLoader
{
	__DeclareClass(CMeshLoaderNVX2);

public:

	// GPU, Access, MipCount (-1 for from-file or full)

	virtual ~CMeshLoaderNVX2() {}

	virtual PResourceLoader	Clone();
	virtual bool			IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool			Load(CResource& Resource);
};

typedef Ptr<CMeshLoaderNVX2> PMeshLoaderNVX2;

}

#endif
