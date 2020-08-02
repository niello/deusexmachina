#pragma once
#include <Animation/PoseOutput.h>
#include <Scene/SceneNode.h>

// Transform output binding to a scene node hierarchy. Used for writing animation result
// into the scene graph. Port 0 is always a hierarchy root. No multiple roots are supported yet.

namespace DEM::Anim
{
using PSkeleton = Ptr<class CSkeleton>;

class CSkeleton : public IPoseOutput
{
protected:

	// root node at port 0 always?
	//???weak ptr?
	//???store mapping inside for root rebinding?
	std::vector<Scene::PSceneNode> _Nodes;

public:

	void SetRootNode(Scene::CSceneNode* pNode);

	virtual U16  BindNode(CStrID NodeID, U16 ParentPort) override;
	virtual U8   GetActivePortChannels(U16 Port) const override;

	virtual void SetScale(U16 Port, const vector3& Scale) override;
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override;
	virtual void SetTranslation(U16 Port, const vector3& Translation) override;
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override;
};

}
