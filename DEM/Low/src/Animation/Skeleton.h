#pragma once
#include <Animation/PoseOutput.h>
#include <Scene/SceneNode.h>

// Transform output binding to a scene node hierarchy. Used for writing animation result
// into the scene graph. Port 0 is always a hierarchy root. No multiple roots are supported yet.

namespace DEM::Anim
{
using PSkeleton = std::unique_ptr<class CSkeleton>;
class CSkeletonInfo;
class CPoseBuffer;

class CSkeleton : public IPoseOutput
{
protected:

	//???weak ptrs?
	std::vector<Scene::PSceneNode> _Nodes;

public:

	void Init(Scene::CSceneNode& Root, const CSkeletonInfo& Info);
	void Clear() { _Nodes.clear();  }

	U16  FindPortByName(CStrID NodeID) const;

	void FromPoseBuffer(const CPoseBuffer& Pose);
	void ToPoseBuffer(CPoseBuffer& Pose) const;

	//!!!can merge channels with a per-bone layer mask into the single feature!
	virtual U8   GetActivePortChannels(U16 Port) const override;

	virtual void SetScale(U16 Port, const vector3& Scale) override;
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override;
	virtual void SetTranslation(U16 Port, const vector3& Translation) override;
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override;
};

}
