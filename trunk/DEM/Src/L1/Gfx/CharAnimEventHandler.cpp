#include "CharAnimEventHandler.h"

#include <Audio/Event/PlaySound.h>
//#include <vfx/Event/PlayVFX.h>
#include <Events/EventManager.h>

extern const matrix44 Rotate180;

namespace Graphics
{

// FIXME: It can happen that events are generated although the game is
// currently paused. When the game is paused, the time stamp will
// remain at one position and the events will be fired each frame.
void CCharAnimEHandler::HandleEvent(const nAnimEventTrack& Track, int EventIdx)
{
	const nAnimEvent& AnimEvt = Track.GetEvent(EventIdx);

	matrix44 Tfm;
	Tfm.mult_simple(matrix44(AnimEvt.GetQuaternion()));
	Tfm.translate(AnimEvt.GetTranslation());
	Tfm.mult_simple(Rotate180);
	Tfm.mult_simple(Entity->GetTransform());

	nTime CurrTime = Entity->GetEntityTime();

	if (LastSoundTime != CurrTime)
	{
		Ptr<Event::PlaySound> Evt = Event::PlaySound::Create();
		Evt->Name = Track.GetName();
		Evt->Position = Tfm.pos_component();
		EventMgr->FireEvent(*Evt);
		LastSoundTime = CurrTime;
	}

	if (LastVFXTime != CurrTime)
	{
		/*
		Ptr<Event::PlayVFX> Evt = Event::PlayVFX::Create();
		Evt->SetName(Track.GetName());
		Evt->SetTransform(Tfm);
		EventMgr->FireEvent(*Evt);
		*/
		LastVFXTime = CurrTime;
	}
}
//---------------------------------------------------------------------

};
