#pragma once
#ifndef __DEM_L2_AI_STIMULUS_H__
#define __DEM_L2_AI_STIMULUS_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Data/QuadTree.h>
#include <Data/KeyList.h>

// Stimulus is a source of sense, placed in the world and collected by actor sensors.
// Derive from this class to create different stimulus types (visual, audial, tactile etc).

namespace AI
{
typedef Data::CKeyList<Core::CRTTI*, class CStimulus*> CStimulusListSet;
typedef Data::CQuadTree<class CStimulus*, CStimulusListSet> CStimulusQT;
typedef CStimulusQT::CHandle CStimulusNode;

class CStimulus: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	//!!!only for external ones!
	CStimulusQT::CNode*	pQTNode;

	//!!!don't create immediately expiring stimuli if sensor that processes them updates too infrequently!
	//???sensors update every frame, perceptors update periodically?
	//!!!can cache potentially sensed stimuli as strong refs in the sensor so they will not be skipped!

public:

	CStrID	SourceEntityID;
	vector3	Position;
	//Radius, Height (cyl) / Radius (sph)
	float	Radius; //???if Height < 0, shp, else cyl?
	float	Intensity;
	float	ExpireTime;		//???is dependent on intensity? //???int?

	CStimulus(): pQTNode(NULL), Radius(0.f) {}

	bool				IsActive() const { return pQTNode != NULL; }
	
	// For CKeyList
	Core::CRTTI*		GetKey() const { return GetRTTI(); }

	// For CQuadTree
	void				GetCenter(vector2& Out) const { Out.x = Position.x; Out.y = Position.z; }
	void				GetHalfSize(vector2& Out) const { Out.x = Out.y = Radius; }
	CStimulusQT::CNode*	GetQuadTreeNode() const { return pQTNode; }
	void				SetQuadTreeNode(CStimulusQT::CNode* pNode) { pQTNode = pNode; }
};

typedef Ptr<CStimulus> PStimulus;

}

#endif