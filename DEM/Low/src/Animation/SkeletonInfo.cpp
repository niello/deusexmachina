#include "SkeletonInfo.h"
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

CSkeletonInfo::CSkeletonInfo(std::vector<CNodeInfo>&& Map)
	: _Map(std::move(Map))
{
#ifdef _DEBUG
	// Mapping must guarantee that child nodes will go after their parents, for parents
	// to be processed before children when map clip bones to output ports
	for (size_t i = 0; i < _Map.size(); ++i)
	{
		n_assert(_Map[i].ParentIndex == EmptyPort || _Map[i].ParentIndex < i);
	}
#endif
}
//---------------------------------------------------------------------

static inline void MapPort(std::vector<U16>& OutPorts, U16 Port, U16 MappedPort)
{
	const bool DirectMapping = OutPorts.empty();
	if (!DirectMapping || Port != MappedPort)
	{
		if (DirectMapping)
		{
			// Direct mapping satisfies our data no more, build explicit mapping
			OutPorts.resize(Port);
			for (U16 j = 0; j < Port; ++j)
				OutPorts[j] = j;
		}

		OutPorts.push_back(MappedPort);
		n_assert_dbg(OutPorts.size() == (Port + 1));
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::MapTo(IPoseOutput& Output, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	for (size_t i = 0; i < _Map.size(); ++i)
	{
		const auto& NodeInfo = _Map[i];

		const bool DirectMapping = OutPorts.empty();

		// See guarantee comment in the constructor
		const U16 ParentOutputPort =
			(NodeInfo.ParentIndex == EmptyPort) ? IPoseOutput::InvalidPort :
			DirectMapping ? NodeInfo.ParentIndex :
			OutPorts[NodeInfo.ParentIndex];

		const auto OutputPort = Output.BindNode(NodeInfo.ID, ParentOutputPort);
		//if (OutputPort == IPoseOutput::InvalidPort) continue with port = IPoseOutput::InvalidPort;

		// Allocate new port
		MapPort(OutPorts, static_cast<U16>(i), OutputPort);
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::MapTo(const CSkeletonInfo& Other, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	// Start from 1 because roots always match
	for (size_t i = 1; i < _Map.size(); ++i)
	{
		const auto& NodeInfo = _Map[i];

		const bool DirectMapping = OutPorts.empty();

		// Find parent of the current node in Other
		U16 OtherParentIndex;
		if (NodeInfo.ParentIndex == EmptyPort)
			OtherParentIndex = 0; // All non-root nodes without a parent are evaluated from the root
		else if (DirectMapping)
			OtherParentIndex = NodeInfo.ParentIndex;
		else
		{
			OtherParentIndex = OutPorts[NodeInfo.ParentIndex];
			if (OtherParentIndex == EmptyPort)
			{
				// Parent of this node was not found, so neither could be the node itself
				OutPorts.push_back(EmptyPort);
				n_assert_dbg(OutPorts.size() == (i + 1));
				continue;
			}
		}

		// TODO: if it is a path, need to process all nodes one by one!
		// FIXME: check if there is a dot in the name instead of checking ParentIndex!
		//if (NodeInfo.ParentIndex == EmptyPort)
		//{
		//}

		// Parent found, scan all its children in Other to find the current node itself
		U16 OtherPort = EmptyPort;
		for (size_t j = OtherParentIndex + 1; j < Other._Map.size(); ++j)
		{
			const auto& OtherNodeInfo = Other._Map[j];
			if (OtherNodeInfo.ParentIndex == OtherParentIndex && NodeInfo.ID == OtherNodeInfo.ID)
			{
				OtherPort = static_cast<U16>(j);
				break;
			}
		}

		MapPort(OutPorts, static_cast<U16>(i), OtherPort);
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::MergeInto(CSkeletonInfo& Other, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	// Start from 1 because roots always match
	for (size_t i = 1; i < _Map.size(); ++i)
	{
		const auto& NodeInfo = _Map[i];

		const bool DirectMapping = OutPorts.empty();

		// Find parent of the current node in Other
		U16 OtherParentIndex;
		if (NodeInfo.ParentIndex == EmptyPort)
			OtherParentIndex = 0; // All non-root nodes without a parent are evaluated from the root
		else if (DirectMapping)
			OtherParentIndex = NodeInfo.ParentIndex;
		else
		{
			OtherParentIndex = OutPorts[NodeInfo.ParentIndex];
			if (OtherParentIndex == EmptyPort)
			{
				//!!!merge parent etc!
			}
		}

		// TODO: if it is a path, need to process all nodes one by one!
		// FIXME: check if there is a dot in the name instead of checking ParentIndex!
		//if (NodeInfo.ParentIndex == EmptyPort)
		//{
		//}

		// Parent found, scan all its children in Other to find the current node itself
		U16 OtherPort = EmptyPort;
		for (size_t j = OtherParentIndex + 1; j < Other._Map.size(); ++j)
		{
			const auto& OtherNodeInfo = Other._Map[j];
			if (OtherNodeInfo.ParentIndex == OtherParentIndex && NodeInfo.ID == OtherNodeInfo.ID)
			{
				OtherPort = static_cast<U16>(j);
				break;
			}
		}

		if (OtherPort == EmptyPort)
		{
			//!!!merge!
		}

		MapPort(OutPorts, static_cast<U16>(i), OtherPort);
	}
}
//---------------------------------------------------------------------

}
