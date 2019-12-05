#pragma once
#include <AI/Perception/Perceptor.h>

// Processes visual and aural info and detects overseer actors. This can be used for a discipline
// emulation, like military subordination, slavery or novitiate.

namespace AI
{

class CPerceptorOverseer: public CPerceptor
{
	FACTORY_CLASS_DECL;

protected:

	CArray<CStrID> Overseers;
	
	//!!!can also have a personal rule (piece of code/script) to detect overseers!

public:

	virtual void Init(const Data::CParams& Desc);
	virtual void ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence = 1.f);
};

}
