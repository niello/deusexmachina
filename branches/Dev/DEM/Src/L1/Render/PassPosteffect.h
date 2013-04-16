#pragma once
#ifndef __DEM_L1_RENDER_PASS_POSTEFFECT_H__
#define __DEM_L1_RENDER_PASS_POSTEFFECT_H__

#include <Render/Pass.h>

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

public:

	virtual void Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights);
	// Render: apply shader vars, clear RT, if necessary, begin pass, render batches, end pass
};

typedef Ptr<CPassPosteffect> PPassPosteffect;

}

#endif
