#pragma once
#ifndef __DEM_L1_RENDER_PASS_POSTEFFECT_H__
#define __DEM_L1_RENDER_PASS_POSTEFFECT_H__

#include <Render/Pass.h>

//!!!OLD!
#include "gfx2/nmesh2.h"

// Renders fullscreen quad, using some source textures and posteffect shader

namespace Render
{

class CPassPosteffect: public CPass
{
	//DeclareRTTI;

//protected:
public:

//???can use MRT?
// Input:
// Textures (including RTs of previous passes), posteffect params
// Output:
// RT with a postprocessed image

//!!!OLD!
    nRef<nMesh2> refQuadMesh;

public:

	~CPassPosteffect();

	virtual void Render();
	// Render: apply shader vars, clear RT, if necessary, begin pass, render batches, end pass

	//!!!OLD!
	virtual void Validate();
	void UpdateMeshCoords();
};

typedef Ptr<CPassPosteffect> PPassPosteffect;

}

#endif
