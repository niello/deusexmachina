#pragma once
#ifndef __DEM_L1_SNV_VALIDATE_RESOURCES_H__
#define __DEM_L1_SNV_VALIDATE_RESOURCES_H__

#include <Scene/NodeVisitor.h>
#include <Data/Ptr.h>

// Scene traversal that validates resources of render objects in a scene subtree

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Frame
{

class CSceneNodeValidateResources: public Scene::INodeVisitor
{
public:

	Render::PGPUDriver GPU;	// Host GPU for VRAM resources

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif
