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

U16 CSkeletonInfo::FindNodePort(CStrID ID) const
{
	for (size_t i = 0; i < _Nodes.size(); ++i)
		if (_Nodes[i].ID == ID)
			return static_cast<U16>(i);

	return EmptyPort;
}
//---------------------------------------------------------------------

// NB: if mapping is direct (each index is bound to the port with the same index),
// then OutPorts will be empty, and user can write to the output without remapping.
void CSkeletonInfo::MapTo(const CSkeletonInfo& Other, std::unique_ptr<U16[]>& OutMapping) const
{
	OutMapping.reset();

	// Start from 1 because roots always match
	for (size_t i = 1; i < _Nodes.size(); ++i)
	{
		const auto& NodeInfo = _Nodes[i];

		// Find parent of the current node in Other
		U16 OtherParentIndex;
		if (NodeInfo.ParentIndex == EmptyPort)
			OtherParentIndex = 0; // All non-root nodes without a parent are evaluated from the root
		else if (!OutMapping)
			OtherParentIndex = NodeInfo.ParentIndex;
		else
		{
			OtherParentIndex = OutMapping[NodeInfo.ParentIndex];
			if (OtherParentIndex == EmptyPort)
			{
				// Parent of this node was not found, so neither could be the node itself
				OutMapping[i] = EmptyPort;
				continue;
			}
		}

		// Parent found, try to find a child node by ID
		const U16 MappedPort = Other.FindNodePort(OtherParentIndex, NodeInfo.ID);
		const U16 Port = static_cast<U16>(i);
		if (OutMapping || Port != MappedPort)
		{
			if (!OutMapping)
			{
				// Direct mapping satisfies our data no more, build explicit mapping
				OutMapping.reset(new U16[_Nodes.size()]);
				for (U16 j = 0; j < Port; ++j)
					OutMapping[j] = j;
			}

			OutMapping[Port] = MappedPort;
		}
	}
}
//---------------------------------------------------------------------

// Use it after MapTo if some ports didn't map. It adds missing ports from this skeleton to Other.
void CSkeletonInfo::MergeInto(CSkeletonInfo& Other, std::unique_ptr<U16[]>& Mapping, size_t FirstEmptyPort) const
{
	if (!Mapping) return;

	for (size_t i = FirstEmptyPort; i < _Nodes.size(); ++i)
	{
		if (Mapping[i] != EmptyPort) continue;

		const auto& NodeInfo = _Nodes[i];

		const U16 OtherParentIndex = (NodeInfo.ParentIndex == EmptyPort) ? EmptyPort : Mapping[NodeInfo.ParentIndex];

		// TODO: if ID is a path, should add all nodes one by one, to avoid referencing the same node in multiple ports!
		//!!!should also try to detect intermediate nodes, they can exist!
		Mapping[i] = static_cast<U16>(Other._Nodes.size());
		Other._Nodes.push_back({ NodeInfo.ID, OtherParentIndex });
	}
}
//---------------------------------------------------------------------

void CSkeletonInfo::Combine(PSkeletonInfo& Dest, CSkeletonInfo& Src, std::unique_ptr<U16[]>& Mapping)
{
	Mapping.reset();

	if (!Dest)
	{
		// Share a source skeleton without copying
		Dest = &Src;
	}
	else if (Dest != &Src)
	{
		Src.MapTo(*Dest, Mapping);

		if (Mapping)
		{
			const auto EndIt = Mapping.get() + Src._Nodes.size();
			const auto EmptyIt = std::find(Mapping.get(), EndIt, CSkeletonInfo::EmptyPort);
			if (EmptyIt != EndIt)
			{
				// Create our own copy instead of modifying a shared one
				if (Dest->GetRefCount() > 1) Dest = n_new(CSkeletonInfo(*Dest));

				const auto StartIdx = static_cast<size_t>(EmptyIt - Mapping.get());
				Src.MergeInto(*Dest, Mapping, StartIdx);
			}
		}
	}
}
//---------------------------------------------------------------------

}
