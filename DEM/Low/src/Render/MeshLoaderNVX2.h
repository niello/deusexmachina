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

	virtual ~CMeshLoaderNVX2() {}

	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CMeshLoaderNVX2> PMeshLoaderNVX2;

}

#endif
