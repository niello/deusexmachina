#pragma once
#include <Scene/NodeVisitor.h>
#include <Data/Array.h>

// Scene traversal that updates render object attributes' spatial information

namespace Scene
{
	class CSPS;
}

namespace Frame
{

class CSceneNodeUpdateInSPS: public Scene::INodeVisitor
{
protected:

	Scene::CSPS& _SPS;

public:

	CSceneNodeUpdateInSPS(Scene::CSPS& SPS) : _SPS(SPS) {}

	virtual bool Visit(Scene::CSceneNode& Node);
};

}
