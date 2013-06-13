#include "AnimClip.h"

#include <Events/EventManager.h>

namespace Anim
{
__ImplementClassNoFactory(Anim::CAnimClip, Resources::CResource);

void CAnimClip::FireEvents(float ExactTime, bool Loop, Events::CEventDispatcher* pDisp, Data::PParams Params) const
{
	if (!EventTracks.GetCount()) return;

	ExactTime = AdjustTime(ExactTime, Loop);

	if (!pDisp) pDisp = EventMgr;

	for (int i = 0; i < EventTracks.GetCount(); ++i)
	{
		CEventTrack& Track = EventTracks[i];

		// [ExactTime, ExactTime]
		int j = 0;
		for (; j < Track.Keys.GetCount(); ++j)
			if (Track.Keys[j].Time >= ExactTime) break;

		for (; j < Track.Keys.GetCount(); ++j)
		{
			CEventTrack::CKey& Key = Track.Keys[j];
			if (Key.Time > ExactTime) break;
			pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
		}
	}
}
//---------------------------------------------------------------------

// Time is unadjusted, so looping doesn't confuse us when we deternime time direction
void CAnimClip::FireEvents(float StartTime, float EndTime, bool Loop, Events::CEventDispatcher* pDisp, Data::PParams Params) const
{
	if (!EventTracks.GetCount()) return;

	bool Forward = EndTime > StartTime;
	StartTime = AdjustTime(StartTime, Loop);
	EndTime = AdjustTime(EndTime, Loop);
	if (StartTime == EndTime) return;

	if (!pDisp) pDisp = EventMgr;

	for (int i = 0; i < EventTracks.GetCount(); ++i)
	{
		CEventTrack& Track = EventTracks[i];

		//!!!binary search closest j!

		if (Forward)
		{
			if (StartTime > EndTime)
			{
				// (StartTime, Duration]
				int j = 0;
				for (; j < Track.Keys.GetCount(); ++j)
					if (Track.Keys[j].Time > StartTime) break;

				for (; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}

				// [0, EndTime]
				for (j = 0; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time > EndTime) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
			else
			{
				// (StartTime, EndTime]
				int j = 0;
				for (; j < Track.Keys.GetCount(); ++j)
					if (Track.Keys[j].Time > StartTime) break;

				for (; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time > EndTime) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
		}
		else
		{
			if (StartTime < EndTime)
			{
				// (StartTime, 0]
				int j = Track.Keys.GetCount() - 1;
				for (; j >= 0 ; --j)
					if (Track.Keys[j].Time < StartTime) break;

				for (; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}

				// [Duration, EndTime]
				for (j = Track.Keys.GetCount() - 1; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time < EndTime) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
			else
			{
				// (StartTime, EndTime]
				int j = Track.Keys.GetCount() - 1;
				for (; j >= 0 ; --j)
					if (Track.Keys[j].Time < StartTime) break;

				for (; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time < EndTime) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
		}
	}
}
//---------------------------------------------------------------------

}