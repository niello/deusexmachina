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

	PSkeletonInfo                    _SkeletonInfo;
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CPoseRecorder(PSkeletonInfo&& SkeletonInfo)
		: _SkeletonInfo(std::move(SkeletonInfo))
	{
		_Transforms.resize(_SkeletonInfo->GetNodeCount());
	}

	PStaticPose GetPose()
	{
		std::vector<Math::CTransformSRT> PoseTransforms;
		std::swap(_Transforms, PoseTransforms);
		return PStaticPose(n_new(CStaticPose(std::move(PoseTransforms), PSkeletonInfo(_SkeletonInfo))));
	}

	virtual void SetScale(U16 Port, const vector3& Scale) override { _Transforms[Port].Scale = Scale; }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { _Transforms[Port].Rotation = Rotation; }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { _Transforms[Port].Translation = Translation; }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { _Transforms[Port] = Tfm; }
};

}
