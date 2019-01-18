#pragma once
#include <Resources/ResourceCreator.h>

// A class of generators that procedurally create CMesh objects, along
// with some most commonly used implementations.

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CMeshGenerator: public IResourceCreator
{
public:

	Render::PGPUDriver	GPU;
	bool				FrontClockWise = true;

	CMeshGenerator();
	virtual ~CMeshGenerator();

	virtual const Core::CRTTI& GetResultType() const override;
};

typedef Ptr<CMeshGenerator> PMeshGenerator;

class CMeshGeneratorQuadPatch: public CMeshGenerator
{
public:

	UPTR QuadsPerEdge;

	virtual PResourceObject CreateResource(CStrID UID) override;
};

typedef Ptr<CMeshGeneratorQuadPatch> PMeshGeneratorQuadPatch;

//???rename to box? use for placeholder boxes too?
class CMeshGeneratorSkybox: public CMeshGenerator
{
public:

	virtual PResourceObject CreateResource(CStrID UID) override;
};

typedef Ptr<CMeshGeneratorSkybox> PMeshGeneratorSkybox;

}
