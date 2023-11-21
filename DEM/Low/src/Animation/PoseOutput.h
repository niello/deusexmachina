#pragma once
#include <StdDEM.h>
#include <rtm/qvvf.h>

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

	virtual U8   GetActivePortChannels(U16 Port) const { return ETransformChannel::All; } // returns ETransformChannel flags that port accepts now

	//???PERF: or SetTransform(tfm, ETransformChannel mask)? 3 bool checks inside, but mb less virtual calls. Is critical?
	virtual void SetScale(U16 Port, const rtm::vector4f& Scale) = 0;
	virtual void SetRotation(U16 Port, const rtm::quatf& Rotation) = 0;
	virtual void SetTranslation(U16 Port, const rtm::vector4f& Translation) = 0;
	virtual void SetTransform(U16 Port, const rtm::qvvf& Tfm) = 0;

	bool         IsPortActive(U16 Port) const { return !!GetActivePortChannels(Port); }
	bool         IsScalingActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Scaling; }
	bool         IsRotationActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Rotation; }
	bool         IsTranslationActive(U16 Port) const { return GetActivePortChannels(Port) & ETransformChannel::Translation; }
};

}
