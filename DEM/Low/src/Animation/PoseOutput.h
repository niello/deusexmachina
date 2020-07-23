#pragma once
#include <Math/TransformSRT.h>
#include <Data/StringID.h>

// Interface for writing a set of 3D transforms to a set of targets (for example, bone nodes)

namespace DEM::Anim
{
using PPoseOutput = std::unique_ptr<class IPoseOutput>;

enum ETransformChannel : U8
{
	Translation	= 0x01,
	Rotation	= 0x02,
	Scaling		= 0x04,
	All         = (Translation | Rotation | Scaling)
};

class IPoseOutput
{
public:

	constexpr static inline U16 InvalidPort = std::numeric_limits<U16>().max();

	virtual U16  BindNode(CStrID NodeID, U16 ParentPort) = 0;
	virtual U8   GetActivePortChannels(U16 Port) const = 0; // returns ETransformChannel flags that port accepts now

	//???PERF: or SetTransform(tfm, ETransformChannel mask)? 3 bool checks inside, but mb less virtual calls. Is critical?
	virtual void SetScale(U16 Port, const vector3& Scale) = 0;
	virtual void SetRotation(U16 Port, const quaternion& Rotation) = 0;
	virtual void SetTranslation(U16 Port, const vector3& Translation) = 0;
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) = 0;

	bool         IsPortActive(U16 Port) const { return !!GetActivePortChannels(Port); }
	bool         IsScalingActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Scaling; }
	bool         IsRotationActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Rotation; }
	bool         IsTranslationActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Translation; }
};

}
