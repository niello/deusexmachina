#include "NodeMapping.h"
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

CNodeMapping::CNodeMapping(std::vector<CNodeInfo>&& Map)
	: _Map(std::move(Map))
{
#ifdef _DEBUG
	// Mapping must guarantee that child nodes will go after their parents, for parents
	// to be processed before children when map clip bones to output ports
	for (size_t i = 0; i < _Map.size(); ++i)
	{
		n_assert(_Map[i].ParentIndex == NoParentIndex || _Map[i].ParentIndex < i);
	}
#endif
}
//---------------------------------------------------------------------

// NB: if mapping is direct (each index is bound to the port with the same index),
// then OutPorts will be cleared and user should write to the output without remapping.
// It happens quite often with animation clips.
void CNodeMapping::Bind(IPoseOutput& Output, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	for (size_t i = 0; i < _Map.size(); ++i)
	{
		const auto& NodeInfo = _Map[i];

		const bool DirectMapping = OutPorts.empty();

		// See guarantee comment in the constructor
		const U16 ParentOutputPort =
			(NodeInfo.ParentIndex == NoParentIndex) ? IPoseOutput::InvalidPort :
			DirectMapping ? NodeInfo.ParentIndex :
			OutPorts[NodeInfo.ParentIndex];

		const auto OutputPort = Output.BindNode(NodeInfo.ID, ParentOutputPort);
		//if (OutputPort == IPoseOutput::InvalidPort) continue with port = IPoseOutput::InvalidPort;

		// Allocate new port
		const auto Port = static_cast<U16>(i);
		if (!DirectMapping || Port != OutputPort)
		{
			if (DirectMapping)
			{
				// Direct mapping satisfies our data no more, build explicit mapping
				OutPorts.resize(Port);
				for (U16 j = 0; j < Port; ++j)
					OutPorts[j] = j;
			}

			OutPorts.push_back(OutputPort);
			n_assert_dbg(OutPorts.size() == (i + 1));
		}
	}
}
//---------------------------------------------------------------------

}
