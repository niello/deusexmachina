#pragma once
#include <Resources/ResourceCreator.h>

// A class of generators that procedurally create CMeshData objects, along
// with some most commonly used implementations.

namespace Resources
{

class CMeshGenerator: public IResourceCreator
{
protected:

	bool _FrontClockWise = false;

public:

	CMeshGenerator(bool FrontClockWise = false) : _FrontClockWise(FrontClockWise) {}

	virtual const Core::CRTTI& GetResultType() const override;
};

class CMeshGeneratorQuadPatch: public CMeshGenerator
{
private:

	U32 _QuadsPerEdge = 4;

public:

	CMeshGeneratorQuadPatch(U32 QuadsPerEdge, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _QuadsPerEdge(QuadsPerEdge) {}

	virtual PResourceObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorBox: public CMeshGenerator
{
public:

	CMeshGeneratorBox(bool FrontClockWise = false) : CMeshGenerator(FrontClockWise) {}

	virtual PResourceObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorSphere: public CMeshGenerator
{
private:

	U16 _LineCount = 4;

public:

	CMeshGeneratorSphere(U16 LineCount, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _LineCount(std::max<U16>(LineCount, 4)) {}

	virtual PResourceObject CreateResource(CStrID UID) override;
};

}
