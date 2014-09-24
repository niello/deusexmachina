#pragma once
#ifndef __DEM_L1_SNV_UPDATE_IN_SPS_H__
#define __DEM_L1_SNV_UPDATE_IN_SPS_H__

#include <Scene/NodeVisitor.h>

// Scene traversal that updates render object attributes' spatial information

namespace Render
{

class CSceneNodeUpdateInSPS: public Scene::INodeVisitor
{
public:

	//!!!can store `in` SPS and `out` arrays of objects always visible!

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif
