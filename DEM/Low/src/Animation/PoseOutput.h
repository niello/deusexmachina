#pragma once
#include <Math/TransformSRT.h>
#include <Data/StringID.h>

// Interface for writing 3D transforms to multiple targets (for example, bone nodes)

namespace DEM::Anim
{
using PPoseOutput = std::unique_ptr<class IPoseOutput>;

class IPoseOutput
{
public:

	constexpr static inline U16 InvalidPort = std::numeric_limits<U16>().max();

	virtual U16  BindNode(CStrID NodeID, U16 ParentPort) = 0;
	virtual bool IsPortActive(U16 Port) const = 0; //???return channel mask instead? S,R,T

	virtual void SetScale(U16 Port, const vector3& Scale) = 0;
	virtual void SetRotation(U16 Port, const quaternion& Rotation) = 0;
	virtual void SetTranslation(U16 Port, const vector3& Translation) = 0;
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) = 0; //???need or inline S+R+T?
};

}
