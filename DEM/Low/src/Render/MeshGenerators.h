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

	virtual const DEM::Core::CRTTI& GetResultType() const override;
};

class CMeshGeneratorQuadPatch: public CMeshGenerator
{
private:

	U32 _QuadsPerEdge = 4;

public:

	CMeshGeneratorQuadPatch(U32 QuadsPerEdge, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _QuadsPerEdge(QuadsPerEdge) {}

	virtual DEM::Core::PObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorBox: public CMeshGenerator
{
public:

	CMeshGeneratorBox(bool FrontClockWise = false) : CMeshGenerator(FrontClockWise) {}

	virtual DEM::Core::PObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorSphere: public CMeshGenerator
{
private:

	U16 _RowCount = 4;

public:

	CMeshGeneratorSphere(U16 RowCount, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _RowCount(std::max<U16>(RowCount, 4)) {}

	virtual DEM::Core::PObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorCylinder: public CMeshGenerator
{
private:

	U16 _SectorCount = 3;

public:

	CMeshGeneratorCylinder(U16 SectorCount, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _SectorCount(std::max<U16>(SectorCount, 3)) {}

	virtual DEM::Core::PObject CreateResource(CStrID UID) override;
};

class CMeshGeneratorCone: public CMeshGenerator
{
private:

	U16 _SectorCount = 3;

public:

	CMeshGeneratorCone(U16 SectorCount, bool FrontClockWise = false) : CMeshGenerator(FrontClockWise), _SectorCount(std::max<U16>(SectorCount, 3)) {}

	virtual DEM::Core::PObject CreateResource(CStrID UID) override;
};

}
