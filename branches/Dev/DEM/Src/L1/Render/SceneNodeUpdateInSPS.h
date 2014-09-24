#pragma once
#ifndef __DEM_L1_SNV_UPDATE_IN_SPS_H__
#define __DEM_L1_SNV_UPDATE_IN_SPS_H__

#include <Scene/NodeVisitor.h>
#include <Data/Array.h>

// Scene traversal that updates render object attributes' spatial information

namespace Render
{
class CSPS;
class CRenderObject;
class CLight;

class CSceneNodeUpdateInSPS: public Scene::INodeVisitor
{
public:

	CSPS*					pSPS;
	CArray<CRenderObject*>*	pVisibleObjects;
	CArray<CLight*>*		pVisibleLights;

	CSceneNodeUpdateInSPS(): pSPS(NULL), pVisibleObjects(NULL), pVisibleLights(NULL) {} 

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif
