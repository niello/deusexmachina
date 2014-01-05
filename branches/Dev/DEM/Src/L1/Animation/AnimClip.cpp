#include "AnimClip.h"

#include <Events/EventServer.h>

namespace Anim
{
__ImplementClassNoFactory(Anim::CAnimClip, Resources::CResource);

void CAnimClip::FireEvents(float ExactCursorPos, bool Loop, Events::CEventDispatcher* pDisp, Data::PParams Params) const
{
	if (!EventTracks.GetCount()) return;

	ExactCursorPos = AdjustCursorPos(ExactCursorPos, Loop);

	if (!pDisp) pDisp = EventSrv;

	for (int i = 0; i < EventTracks.GetCount(); ++i)
	{
		CEventTrack& Track = EventTracks[i];

		// [ExactCursorPos, ExactCursorPos]
		DWORD j = 0;
		for (; j < Track.Keys.GetCount(); ++j)
			if (Track.Keys[j].Time >= ExactCursorPos) break;

		for (; j < Track.Keys.GetCount(); ++j)
		{
			CEventTrack::CKey& Key = Track.Keys[j];
			if (Key.Time > ExactCursorPos) break;
			pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
		}
	}
}
//---------------------------------------------------------------------

// Time is unadjusted, so looping doesn't confuse us when we deternime time direction.
// Interval is (StartCursorPos; EndCursorPos], so events at the StartCursorPos aren't fired.
void CAnimClip::FireEvents(float StartCursorPos, float EndCursorPos, bool Loop, Events::CEventDispatcher* pDisp, Data::PParams Params) const
{
	if (!EventTracks.GetCount()) return;

	bool Forward = EndCursorPos > StartCursorPos;
	StartCursorPos = AdjustCursorPos(StartCursorPos, Loop);
	EndCursorPos = AdjustCursorPos(EndCursorPos, Loop);
	if (StartCursorPos == EndCursorPos) return; //???what if full round?

	if (!pDisp) pDisp = EventSrv;

	for (int i = 0; i < EventTracks.GetCount(); ++i)
	{
		CEventTrack& Track = EventTracks[i];

		//!!!binary search closest j!

		if (Forward)
		{
			if (StartCursorPos > EndCursorPos)
			{
				// (StartCursorPos, Duration]
				DWORD j = 0;
				for (; j < Track.Keys.GetCount(); ++j)
					if (Track.Keys[j].Time > StartCursorPos) break;

				for (; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}

				// [0, EndCursorPos]
				for (j = 0; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time > EndCursorPos) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
			else
			{
				// (StartCursorPos, EndCursorPos]
				DWORD j = 0;
				for (; j < Track.Keys.GetCount(); ++j)
					if (Track.Keys[j].Time > StartCursorPos) break;

				for (; j < Track.Keys.GetCount(); ++j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time > EndCursorPos) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
		}
		else
		{
			if (StartCursorPos < EndCursorPos)
			{
				// (StartCursorPos, 0]
				int j = Track.Keys.GetCount() - 1;
				for (; j >= 0 ; --j)
					if (Track.Keys[j].Time < StartCursorPos) break;

				for (; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}

				// [Duration, EndCursorPos]
				for (j = Track.Keys.GetCount() - 1; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time < EndCursorPos) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
			else
			{
				// (StartCursorPos, EndCursorPos]
				int j = Track.Keys.GetCount() - 1;
				for (; j >= 0 ; --j)
					if (Track.Keys[j].Time < StartCursorPos) break;

				for (; j >= 0 ; --j)
				{
					CEventTrack::CKey& Key = Track.Keys[j];
					if (Key.Time < EndCursorPos) break;
					pDisp->FireEvent(Key.EventID, Params); //!!!can add direction here or in anim task!
				}
			}
		}
	}
}
//---------------------------------------------------------------------

}