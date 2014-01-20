#include "SensorVision.h"

#include <AI/PropActorBrain.h>
#include <AI/Perception/Perceptor.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <AI/Movement/Memory/MemFactObstacle.h>
#include <Game/EntityManager.h>
#include <mathlib/sphere.h>

namespace AI
{
__ImplementClass(AI::CSensorVision, 'SEVI', AI::CSensor);

void CSensorVision::Init(const Data::CParams& Desc)
{
	//CSensor::Init(Desc);
	FOV = cosf(n_deg2rad(Desc.Get<int>(CStrID("FOV"), 0)));
	Radius = Desc.Get<float>(CStrID("Radius"), 0.f);
}
//---------------------------------------------------------------------

bool CSensorVision::AcceptsStimulusType(const Core::CRTTI& Type) const
{
	return Type == CStimulusVisible::RTTI;
}
//---------------------------------------------------------------------

bool CSensorVision::SenseStimulus(CActor* pActor, CStimulus* pStimulus) const
{
	if (pStimulus->Intensity <= 0.f) OK;

	// Cast to stimulus visible

	//!!!take size/radius into account:!
	//!!!FOV & size! check pos before as faster check, if ok check bounds by FOV?
	//???check visible shape, not only position point? or get radius of stimulus and check against cylinder?
	//y can be projected (clamped by cylinder top & bottom)
	
	vector3 DirToStimulus = pStimulus->Position - pActor->Position;
	float SqDist = DirToStimulus.SqLength();

	if (SqDist <= Radius * Radius)
	{
		float Confidence = pStimulus->Intensity; //!!!calc by formula, use distance!

		bool IsInFOV = FOV < -0.999f; // For 360 degrees vision
		
		if (!IsInFOV)
		{
			DirToStimulus /= n_sqrt(SqDist);
			IsInFOV = pActor->LookatDir.dot(DirToStimulus) >= FOV;
			//!!!adjust confidence by peripheral vision!
		}

		if (IsInFOV)
		{
			//!!!occlusion & line of sight! other objects can occlude this!
			//partial occlusion can reduce confidence

			if (Confidence > 0.f)
			{
				CArray<PPerceptor>::CIterator ItPerceptor;
				for (ItPerceptor = Perceptors.Begin(); ItPerceptor != Perceptors.End(); ItPerceptor++)
					(*ItPerceptor)->ProcessStimulus(pActor, pStimulus, Confidence);
			}
		}
	}

	//???!!!if time-sliced, fail?!

	OK;
}
//---------------------------------------------------------------------

bool CSensorVision::ValidatesFactType(const Core::CRTTI& Type) const
{
	return Type == CMemFactObstacle::RTTI;
}
//---------------------------------------------------------------------

DWORD CSensorVision::ValidateFact(CActor* pActor, const CMemFact& Fact) const
{
	const CMemFactObstacle& Obstacle = (const CMemFactObstacle&)Fact;

	if (Fact.LastUpdateTime == Fact.LastPerceptioCTime &&
		(!Obstacle.pSourceStimulus ||
		!Obstacle.pSourceStimulus->IsActive() ||
		Obstacle.pSourceStimulus->Intensity <= 0.f)) return Failure;

	//!!!CODE DUPLICATION!
	vector3 DirToFact = Obstacle.Position - pActor->Position;
	float SqDist = DirToFact.SqLength();

	if (SqDist <= Radius * Radius)
	{
		bool IsInFOV = FOV < -0.999f; // For 360 degrees vision
		
		if (!IsInFOV)
		{
			DirToFact /= n_sqrt(SqDist);
			IsInFOV = pActor->LookatDir.dot(DirToFact) >= FOV;
			//!!!if peripheral vision reduces confidence to 0, skip!
		}

		if (IsInFOV)
		{
			//!!!occlusion & line of sight! other objects can occlude this!
			//take into account only complete occlusion

			//if (Confidence > 0.f)
			return Failure;
		}
	}

	// We didn't see source stimulus this frame, so we don't know is fact relevant
	return Running;
}
//---------------------------------------------------------------------

EClipStatus CSensorVision::GetBoxClipStatus(CActor* pActor, const CAABB& Box) const
{
	//???check FOV too?
	sphere Sphere(pActor->Position, Radius);
	return Sphere.GetClipStatus(Box);
}
//---------------------------------------------------------------------

} //namespace AI