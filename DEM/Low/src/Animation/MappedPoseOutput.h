#pragma once
#include <Animation/PoseOutput.h>

// Remaps pose from one set of ports to another. Useful for example when animation clip
// tracks do not directly correspond to skeleton bones.

namespace DEM::Anim
{

// FIXME: stack-only, not suited for refcounted storing!
class CStackMappedPoseOutput : public IPoseOutput
{
protected:

	IPoseOutput& _Output;
	U16*         _PortMapping;

public:

	CStackMappedPoseOutput(IPoseOutput& Output, U16* PortMapping) : _Output(Output), _PortMapping(PortMapping) {}

	virtual U8   GetActivePortChannels(U16 Port) const override { return _Output.GetActivePortChannels(_PortMapping[Port]); }

	virtual void SetScale(U16 Port, const vector3& Scale) override { return _Output.SetScale(_PortMapping[Port], Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { return _Output.SetRotation(_PortMapping[Port], Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { return _Output.SetTranslation(_PortMapping[Port], Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { return _Output.SetTransform(_PortMapping[Port], Tfm); }
};

}
