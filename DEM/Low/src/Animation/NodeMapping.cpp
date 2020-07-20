#pragma once
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

// NB: if mapping is direct (each index is bound to the port with teh same index),
// then OutPorts will be cleared and user should write to the output without remapping.
// It happens quite often with animation clips.
void CNodeMapping::Bind(IPoseOutput& Output, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();
	OutPorts.reserve(_Map.size());

	bool HasShifts = false;
	for (size_t i = 0; i < _Map.size(); ++i)
	{
		const auto& NodeInfo = _Map[i];

		// See guarantee comment in the constructor
		const U16 ParentPort = (NodeInfo.ParentIndex == NoParentIndex) ?
			IPoseOutput::InvalidPort :
			OutPorts[NodeInfo.ParentIndex];

		OutPorts.push_back(Output.BindNode(NodeInfo.ID, ParentPort));
		HasShifts = HasShifts || (OutPorts.back() != i);
	}

	// If source indices directly map to output ports, can skip mapping when decompress pose
	if (!HasShifts) OutPorts.clear();
}
//---------------------------------------------------------------------

}
