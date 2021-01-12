#include "SkeletonInfo.h"
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

CSkeletonInfo::CSkeletonInfo(std::vector<CNodeInfo>&& Nodes)
	: _Nodes(std::move(Nodes))
{
#ifdef _DEBUG
	// Mapping must guarantee that child nodes will go after their parents, for parents
	// to be processed before children when map clip bones to output ports
	for (size_t i = 0; i < _Nodes.size(); ++i)
	{
		n_assert(_Nodes[i].ParentIndex == EmptyPort || _Nodes[i].ParentIndex < i);
	}
#endif
}
//---------------------------------------------------------------------

U16 CSkeletonInfo::FindNodePort(U16 ParentIndex, CStrID ID) const
{
	// TODO: if ID is a path, need to process all nodes one by one!
	//!!!can even pre-detect dots and store a cached bit flag in CNodeInfo!

	for (size_t i = ParentIndex + 1; i < _Nodes.size(); ++i)
		if (_Nodes[i].ParentIndex == ParentIndex && _Nodes[i].ID == ID)
			return static_cast<U16>(i);

	return EmptyPort;
}
//---------------------------------------------------------------------

static inline void MapPort(std::vector<U16>& MappedPorts, U16 Port, U16 MappedPort)
{
	const bool DirectMapping = MappedPorts.empty();
	if (!DirectMapping || Port != MappedPort)
	{
		if (DirectMapping)
		{
			// Direct mapping satisfies our data no more, build explicit mapping
			MappedPorts.resize(Port);
			for (U16 j = 0; j < Port; ++j)
				MappedPorts[j] = j;
		}

		MappedPorts.push_back(MappedPort);
		n_assert_dbg(MappedPorts.size() == (Port + 1));
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::MapTo(IPoseOutput& Output, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	for (size_t i = 0; i < _Nodes.size(); ++i)
	{
		const auto& NodeInfo = _Nodes[i];

		const U16 ParentOutputPort =
			(NodeInfo.ParentIndex == EmptyPort) ? IPoseOutput::InvalidPort :
			OutPorts.empty() ? NodeInfo.ParentIndex :
			OutPorts[NodeInfo.ParentIndex];

		MapPort(OutPorts, static_cast<U16>(i), Output.BindNode(NodeInfo.ID, ParentOutputPort));
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::MapTo(const CSkeletonInfo& Other, std::vector<U16>& OutPorts) const
{
	OutPorts.clear();

	// Start from 1 because roots always match
	for (size_t i = 1; i < _Nodes.size(); ++i)
	{
		const auto& NodeInfo = _Nodes[i];

		// Find parent of the current node in Other
		U16 OtherParentIndex;
		if (NodeInfo.ParentIndex == EmptyPort)
			OtherParentIndex = 0; // All non-root nodes without a parent are evaluated from the root
		else if (OutPorts.empty())
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

		// Parent found, try to find a child node by ID
		MapPort(OutPorts, static_cast<U16>(i), Other.FindNodePort(OtherParentIndex, NodeInfo.ID));
	}
}
//---------------------------------------------------------------------

// Use it after MapTo if some ports didn't map. It adds missing ports from this skeleton to Other.
void CSkeletonInfo::MergeInto(CSkeletonInfo& Other, std::vector<U16>& Ports, size_t FirstEmptyPort) const
{
	for (size_t i = FirstEmptyPort; i < Ports.size(); ++i)
	{
		if (Ports[i] != EmptyPort) continue;

		const auto& NodeInfo = _Nodes[i];

		const U16 OtherParentIndex = (NodeInfo.ParentIndex == EmptyPort) ? EmptyPort : Ports[NodeInfo.ParentIndex];

		// TODO: if ID is a path, should add all nodes one by one, to avoid referencing the same node in multiple ports!
		//!!!should also try to detect intermediate nodes, they can exist!
		Ports[i] = static_cast<U16>(Other._Nodes.size());
		Other._Nodes.push_back({ NodeInfo.ID, OtherParentIndex });
	}
}
//---------------------------------------------------------------------

}
