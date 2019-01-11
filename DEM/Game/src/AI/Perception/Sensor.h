#pragma once
#ifndef __DEM_L2_AI_SENSOR_H__
#define __DEM_L2_AI_SENSOR_H__

#include <Data/Params.h>
#include <AI/ActorFwd.h>
#include <AI/Perception/Perceptor.h>
#include <Math/AABB.h>

// Sensors gather initial information from the world or actor itself either by event listening or
// polling (or both). Sensor just reports stimuli sensed by actor. Perceptors then can interpret
// these stimuli as knowledge symbols (facts, objects) and place them into the memory.
// Some internal sensors like SensorDamage rely on actor's state, not on the world stimuli.
// Sensors also can validate facts in memory to clear irrelevant knowledge.

namespace AI
{
class CStimulus;
class CMemFact;

class CSensor: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	// Enabled
	// NextUpdateTime
	// UpdateRate

	//!!!need set instead of array!
	CArray<PPerceptor> Perceptors;

public:

	virtual void		Init(const Data::CParams& Desc) = 0;
	virtual bool		AcceptsStimulusType(const Core::CRTTI& Type) const = 0;
	virtual bool		SenseStimulus(CActor* pActor, CStimulus* pStimulus) const = 0;
	virtual bool		ValidatesFactType(const Core::CRTTI& Type) const { FAIL; }
	virtual UPTR		ValidateFact(CActor* pActor, const CMemFact& Fact) const { return Running; }
	virtual EClipStatus	GetBoxClipStatus(CActor* pActor, const CAABB& Box) const = 0;
	//???bool IsExternal, or hard-split sensors to internal & external?

	void				AddPerceptor(CPerceptor* pPerceptor);
	void				RemovePerceptor(const CPerceptor* pPerceptor);
};

typedef Ptr<CSensor> PSensor;
//---------------------------------------------------------------------

inline void CSensor::AddPerceptor(CPerceptor* pPerceptor)
{
	for (CArray<PPerceptor>::CIterator It = Perceptors.Begin(); It != Perceptors.End(); ++It)
		if ((*It).Get() == pPerceptor) return;
	Perceptors.Add(pPerceptor);
}
//---------------------------------------------------------------------

inline void CSensor::RemovePerceptor(const CPerceptor* pPerceptor)
{
	for (CArray<PPerceptor>::CIterator It = Perceptors.Begin(); It != Perceptors.End(); ++It)
		if ((*It).Get() == pPerceptor)
		{
			Perceptors.Remove(It);
			return;
		}
}
//---------------------------------------------------------------------

}

#endif