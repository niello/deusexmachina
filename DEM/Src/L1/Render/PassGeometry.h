#pragma once
#ifndef __DEM_L1_RENDER_PASS_GEOMETRY_H__
#define __DEM_L1_RENDER_PASS_GEOMETRY_H__

#include <Render/Pass.h>

//!!!OLD!
#include "renderpath/nrpphase.h"

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{

class CPassGeometry: public CPass
{
	//DeclareRTTI;

//protected:
public:

// Input:
// Geometry, lights (if lighting is enabled)
// Output:
// Mid-RT or frame RT

// Batch has batch shader (common), shader vars
// shader feature flags of the batch + batch type + mesh filter
// lighting type (some geoms are rendered unlit)
// sorting type

// Batch can be:
// Scene geometry (default)
// UI
// Text
// Debug shapes
//
//???System UI, Depth buffer resolve to tex, mouse ptrs, lights for prepass

	// BatchRenderer list

//!!!OLD!
	nArray<nRpPhase> phases;

public:

	virtual void Render();
	// Render: apply shader vars, clear RT, if necessary, begin pass, render batches, end pass

	//!!!OLD!
	virtual void Validate();
};

//typedef Ptr<CPass> PPass;

}

#endif
