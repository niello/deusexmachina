#pragma once
#ifndef __DEM_L2_AI_STIMULUS_H__
#define __DEM_L2_AI_STIMULUS_H__

#include <Core/Object.h>
#include <Data/StringID.h>
#include <Data/QuadTree.h>
#include <Data/KeyList.h>

// Stimulus is a source of sense, placed in the world and collected by actor sensors.
// Derive from this class to create different stimulus types (visual, audial, tactile etc).

namespace AI
{
typedef Data::CKeyList<const Core::CRTTI*, class CStimulus*> CStimulusListSet;
typedef Data::CQuadTree<class CStimulus*, CStimulusListSet> CStimulusQT;
typedef CStimulusQT::CHandle CStimulusNode;

class CStimulus: public Core::CObject
{
	RTTI_CLASS_DECL(AI::CStimulus, Core::CObject);

protected:

	//!!!don't create immediately expiring stimuli if sensor that processes them updates too infrequently!
	//???sensors update every frame, perceptors update periodically?
	//!!!can cache potentially sensed stimuli as strong refs in the sensor so they will not be skipped!

public:

	//!!!need only for external stimuli!
	CStimulusQT::CNode*	pQTNode;

	CStrID	SourceEntityID;
	vector3	Position;
	//Radius, Height (cyl) / Radius (sph)
	float	Radius; //???if Height < 0, shp, else cyl?
	float	Intensity;
	float	ExpireTime;		//???is dependent on intensity? //???int?

	CStimulus(): pQTNode(nullptr), Radius(0.f) {}
	~CStimulus() { if (pQTNode) pQTNode->RemoveByValue(this); }

	bool			IsActive() const { return pQTNode != nullptr; }
	
	// For CKeyList
	const Core::CRTTI* GetKey() const { return GetRTTI(); }
};

typedef Ptr<CStimulus> PStimulus;

}

#endif
