#pragma once
#include <Animation/PoseOutput.h>
#include <Scene/SceneNode.h>

// Transform output binding to a scene node hierarchy. Used for writing animation result
// into the scene graph. Port 0 is always a hierarchy root. No multiple roots are supported yet.

namespace DEM::Anim
{
using PSkeleton = Ptr<class CSkeleton>;
class CSkeletonInfo;

class CSkeleton : public IPoseOutput
{
protected:

	// root node at port 0 always?
	//???weak ptr?
	//???store mapping inside for root rebinding?
	std::vector<Scene::PSceneNode> _Nodes;

public:

	void Init(Scene::CSceneNode& Root, const CSkeletonInfo& Info);
	void Clear() { _Nodes.clear();  }

	//!!!can merge into the single feature with a per-bone layer mask!
	virtual U8   GetActivePortChannels(U16 Port) const override;

	virtual void SetScale(U16 Port, const vector3& Scale) override;
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override;
	virtual void SetTranslation(U16 Port, const vector3& Translation) override;
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override;
};

}
