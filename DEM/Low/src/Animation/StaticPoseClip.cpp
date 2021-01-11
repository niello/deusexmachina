#include "StaticPoseClip.h"
#include <Animation/SceneNodeMapping.h>
#include <Animation/MappedPoseOutput.h>

namespace DEM::Anim
{

PPoseClipBase CStaticPoseClip::Clone() const
{
	//!!!pose must be reusable, not unique ptr!
	//PStaticPoseClip NewClip(n_new(CStaticPoseClip()));
	//NewClip->_Pose = _Pose;
	//return NewClip;
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

void CStaticPoseClip::BindToOutput(const PPoseOutput& Output)
{
	if (!_Pose || _Output == Output) return;

	std::vector<U16> PortMapping;
	_Pose->GetNodeMapping().Bind(*Output, PortMapping);
	if (PortMapping.empty())
		_Output = Output;
	else
		_Output = n_new(DEM::Anim::CMappedPoseOutput(PPoseOutput(Output), std::move(PortMapping)));
}
//---------------------------------------------------------------------

void CStaticPoseClip::PlayInterval(float /*PrevTime*/, float /*CurrTime*/, bool IsLast, const CPoseTrack& /*Track*/, UPTR /*ClipIndex*/)
{
	if (IsLast && _Output && _Pose) _Pose->Apply(*_Output);
}
//---------------------------------------------------------------------

}
