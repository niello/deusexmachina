#pragma once
#include <Data/RefCounted.h>
#include <Data/StringID.h>

// Maps an array of transforms (a pose) to a scene node hierarchy (a skeleton).
// Use it to initialize a CSkeleton instance from an user-defined root scene node.
// Refcounted because static poses recorded from animation clips share the same skeleton
// and maybe in the future clips with identical mappings will be able to share it too.
// NB: children always go after their parents. Root node is always the first one.

// TODO: for now path in ID is supported only for records with empty ParentIndex. Allow for any records?
// Can test whole ID first, and if not found, but contain dots, search by path.
//???force that nodes to have a parent 0 (root)? Logically the same.

namespace DEM::Anim
{
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class IPoseOutput;

class CSkeletonInfo : public Data::CRefCounted
{
public:

	constexpr static U16 EmptyPort = std::numeric_limits<U16>().max();

	struct CNodeInfo
	{
		CStrID ID;
		U16    ParentIndex = EmptyPort;
	};

	CSkeletonInfo(std::vector<CNodeInfo>&& Map);

	const UPTR       GetNodeCount() const { return _Map.size(); }
	const CNodeInfo& GetNodeInfo(UPTR Index) const { return _Map[Index]; }

	// NB: if mapping is direct (each index is bound to the port with the same index),
	// then OutPorts will be cleared and user can write to the output without remapping.
	void             MapTo(IPoseOutput& Output, std::vector<U16>& OutPorts) const;
	void             MapTo(const CSkeletonInfo& Other, std::vector<U16>& OutPorts) const;
	void             MergeInto(CSkeletonInfo& Other, std::vector<U16>& OutPorts) const;

protected:

	std::vector<CNodeInfo> _Map;
};

}
