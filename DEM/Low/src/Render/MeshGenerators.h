#pragma once
#include <Resources/ResourceCreator.h>

// A class of generators that procedurally create CMeshData objects, along
// with some most commonly used implementations.

namespace Resources
{

class CMeshGenerator: public IResourceCreator
{
public:

	bool FrontClockWise = true;

	CMeshGenerator();
	virtual ~CMeshGenerator();

	virtual const Core::CRTTI& GetResultType() const override;
};

typedef Ptr<CMeshGenerator> PMeshGenerator;

class CMeshGeneratorQuadPatch: public CMeshGenerator
{
private:

	UPTR _QuadsPerEdge;

public:

	CMeshGeneratorQuadPatch(UPTR QuadsPerEdge) : _QuadsPerEdge(QuadsPerEdge) {}

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
