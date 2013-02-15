#include "MocapClip.h"

namespace Anim
{

bool CMocapClip::Setup(const nArray<CMocapTrack>& _Tracks, const nArray<int>& TrackMapping, vector4* _pKeys)
{
	n_assert(_pKeys);

	Unload();

	pKeys = _pKeys;
	Tracks = _Tracks;

	for (int i = 0; i < Tracks.Size(); ++i)
	{
		int NodeID = TrackMapping[i];
		int CtlrIdx = NodeToCtlr.FindIndex(NodeID);
		PAnimControllerClip Ctlr;
		if (CtlrIdx == INVALID_INDEX)
		{
			Ctlr.Create();
			NodeToCtlr.Add(NodeID, Ctlr);
		}
		else Ctlr = NodeToCtlr.ValueAtIndex(CtlrIdx);
		Ctlr->SetTrack(&Tracks[i], Tracks[i].Channel);
	}

	OK;
}
//---------------------------------------------------------------------

void CMocapClip::Unload()
{
	// Leave users with strong refs and clear controllers, so that
	// they no more reference the data of this resource
	for (int i = 0; i < NodeToCtlr.Size(); ++i)
	{
		NodeToCtlr.ValueAtIndex(i)->Activate(false);
		NodeToCtlr.ValueAtIndex(i)->Clear();
	}

	NodeToCtlr.Clear();
	Tracks.Clear();
	SAFE_DELETE_ARRAY(pKeys);
}
//---------------------------------------------------------------------

}
