#pragma once
#ifndef __DEM_L1_MESH_GENERATORS_H__
#define __DEM_L1_MESH_GENERATORS_H__

#include <Resources/ResourceGenerator.h>

// A class of generators that procedurally create CMesh objects, along
// with some most commonly used implementations.

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CMeshGenerator: public CResourceGenerator
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver	GPU;
	bool				FrontClockWise;

	CMeshGenerator(): FrontClockWise(true) {}

	//virtual ~CMeshGenerator() {}

	virtual const Core::CRTTI& GetResultType() const;
};

typedef Ptr<CMeshGenerator> PMeshGenerator;

class CMeshGeneratorQuadPatch: public CMeshGenerator
{
	__DeclareClassNoFactory;

public:

	UPTR QuadsPerEdge;

	//virtual ~CMeshGeneratorQuadPatch() {}

	virtual PResourceObject Generate();
};

typedef Ptr<CMeshGeneratorQuadPatch> PMeshGeneratorQuadPatch;

//???rename to box? use for paceholder boxes too?
class CMeshGeneratorSkybox: public CMeshGenerator
{
	__DeclareClassNoFactory;

public:

	//virtual ~CMeshGeneratorSkybox() {}

	virtual PResourceObject Generate();
};

typedef Ptr<CMeshGeneratorSkybox> PMeshGeneratorSkybox;

}

#endif
