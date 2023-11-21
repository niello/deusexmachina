#pragma once
#include <Animation/PoseOutput.h>

// Remaps pose from one set of ports to another. Useful for example when animation clip
// tracks do not directly correspond to skeleton bones.
// NB: this output doesn't own its data, the best usage will be to create it on the stack when needed

namespace DEM::Anim
{

class CMappedPoseOutput : public IPoseOutput
{
protected:

	IPoseOutput& _Output;
	U16*         _PortMapping;

public:

	CMappedPoseOutput(IPoseOutput& Output, U16* PortMapping) : _Output(Output), _PortMapping(PortMapping) {}

	virtual U8 GetActivePortChannels(U16 Port) const override
	{
		const U16 MappedPort = _PortMapping[Port];
		return (MappedPort != InvalidPort) ? _Output.GetActivePortChannels(MappedPort) : 0;
	}

	virtual void SetScale(U16 Port, const rtm::vector4f& Scale) override
	{
		const U16 MappedPort = _PortMapping[Port];
		if (MappedPort != InvalidPort) _Output.SetScale(MappedPort, Scale);
	}

	virtual void SetRotation(U16 Port, const rtm::quatf& Rotation) override
	{
		const U16 MappedPort = _PortMapping[Port];
		if (MappedPort != InvalidPort) _Output.SetRotation(MappedPort, Rotation);
	}

	virtual void SetTranslation(U16 Port, const rtm::vector4f& Translation) override
	{
		const U16 MappedPort = _PortMapping[Port];
		if (MappedPort != InvalidPort) _Output.SetTranslation(MappedPort, Translation);
	}

	virtual void SetTransform(U16 Port, const rtm::qvvf& Tfm) override
	{
		const U16 MappedPort = _PortMapping[Port];
		if (MappedPort != InvalidPort) _Output.SetTransform(MappedPort, Tfm);
	}
};

}
