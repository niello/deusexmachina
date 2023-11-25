#pragma once
#include <Animation/PoseOutput.h>
#include <Animation/StaticPose.h>
#include <Animation/SkeletonInfo.h>

// Records a static pose

namespace DEM::Anim
{

class CPoseRecorder : public IPoseOutput
{
protected:

	PSkeletonInfo          _SkeletonInfo;
	std::vector<rtm::qvvf> _Transforms;

public:

	CPoseRecorder(PSkeletonInfo&& SkeletonInfo)
		: _SkeletonInfo(std::move(SkeletonInfo))
	{
		_Transforms.resize(_SkeletonInfo->GetNodeCount());
	}

	PStaticPose GetPose()
	{
		std::vector<rtm::qvvf> PoseTransforms;
		std::swap(_Transforms, PoseTransforms);
		return PStaticPose(n_new(CStaticPose(std::move(PoseTransforms), PSkeletonInfo(_SkeletonInfo))));
	}

	virtual void SetScale(U16 Port, const rtm::vector4f& Scale) override { _Transforms[Port].scale = Scale; }
	virtual void SetRotation(U16 Port, const rtm::quatf& Rotation) override { _Transforms[Port].rotation = Rotation; }
	virtual void SetTranslation(U16 Port, const rtm::vector4f& Translation) override { _Transforms[Port].translation = Translation; }
	virtual void SetTransform(U16 Port, const rtm::qvvf& Tfm) override { _Transforms[Port] = Tfm; }
};

}
