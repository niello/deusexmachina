#pragma once
#ifndef __DEM_L3_AI_SENSOR_VISION_H__
#define __DEM_L3_AI_SENSOR_VISION_H__

#include <AI/Perception/Sensor.h>

// Basic human-like vision in optical spectrum with a FOV

namespace AI
{

class CSensorVision: public CSensor
{
	__DeclareClass(CSensorVision);

protected:

	float FOV;
	float Radius;
	//???attenuation formula?

public:

	virtual void		Init(const Data::CParams& Desc);
	virtual bool		AcceptsStimulusType(const Core::CRTTI& Type) const;
	virtual bool		SenseStimulus(CActor* pActor, CStimulus* pStimulus) const;
	virtual bool		ValidatesFactType(const Core::CRTTI& Type) const;
	virtual EExecStatus	ValidateFact(CActor* pActor, const CMemFact& Fact) const;
	virtual EClipStatus	GetBoxClipStatus(CActor* pActor, const bbox3& Box) const;
};

}

#endif