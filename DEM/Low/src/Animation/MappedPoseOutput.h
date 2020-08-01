#pragma once
#include <Animation/PoseOutput.h>

// Remaps pose from one set of ports to another. Useful for example when animation clip
// tracks do not directly correspond to skeleton bones.

namespace DEM::Anim
{
using PMappedPoseOutput = std::unique_ptr<class CMappedPoseOutput>;

class CMappedPoseOutput : public IPoseOutput
{
protected:

	PPoseOutput      _Output;
	std::vector<U16> _PortMapping;

public:

	CMappedPoseOutput(PPoseOutput&& Output, std::vector<U16>&& PortMapping)
		: _Output(std::move(Output))
		, _PortMapping(std::move(PortMapping))
	{}

	// For replacing direct output when first port with indirect mapping is encountered
	CMappedPoseOutput(PPoseOutput&& Output, UPTR InitialPortCount, U16 FirstIndirectPort)
		: _Output(std::move(Output))
	{
		_PortMapping.resize(InitialPortCount + 1);
		for (UPTR i = 0; i < InitialPortCount; ++i)
			_PortMapping[i] = i;
		_PortMapping[InitialPortCount] = FirstIndirectPort;
	}

	virtual U16 BindNode(CStrID NodeID, U16 ParentPort) override
	{
		const auto OutputPort = _Output->BindNode(NodeID, _PortMapping[ParentPort]);

		// If this output port is already mapped, return existing input port
		auto It = std::find(_PortMapping.cbegin(), _PortMapping.cend(), OutputPort);
		if (It != _PortMapping.cend()) return static_cast<U16>(std::distance(_PortMapping.cbegin(), It));

		// Add new output port to mapping
		_PortMapping.push_back(OutputPort);
		return static_cast<U16>(_PortMapping.size() - 1);
	}

	virtual U8   GetActivePortChannels(U16 Port) const override { return _Output->GetActivePortChannels(_PortMapping[Port]); }

	virtual void SetScale(U16 Port, const vector3& Scale) override { return _Output->SetScale(_PortMapping[Port], Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { return _Output->SetRotation(_PortMapping[Port], Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { return _Output->SetTranslation(_PortMapping[Port], Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { return _Output->SetTransform(_PortMapping[Port], Tfm); }
};

}
