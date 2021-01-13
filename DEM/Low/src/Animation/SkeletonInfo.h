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
//!!!Cases when one skeleton has "a.b.c" and other has "a", "b" & "c" will lead to dangerous merge.
//Either the "c" node will be referenced twice or its parent will be placed after it.
//???Forbid using pathes at all? Or restrict to only certain cases?

namespace DEM::Anim
{
using PSkeletonInfo = Ptr<class CSkeletonInfo>;

class CSkeletonInfo : public Data::CRefCounted
{
public:

	constexpr static U16 EmptyPort = std::numeric_limits<U16>().max();

	struct CNodeInfo
	{
		CStrID ID;
		U16    ParentIndex = EmptyPort;
	};

	static void      Combine(PSkeletonInfo& Dest, CSkeletonInfo& Src, std::unique_ptr<U16[]>& Mapping);

	CSkeletonInfo(std::vector<CNodeInfo>&& Nodes);

	UPTR             GetNodeCount() const { return _Nodes.size(); }
	const CNodeInfo& GetNodeInfo(UPTR Index) const { return _Nodes[Index]; }
	U16              FindNodePort(U16 ParentIndex, CStrID ID) const;
	void             MapTo(const CSkeletonInfo& Other, std::unique_ptr<U16[]>& OutMapping) const;
	void             MergeInto(CSkeletonInfo& Other, std::unique_ptr<U16[]>& Mapping, size_t FirstEmptyPort = 0) const;

protected:

	std::vector<CNodeInfo> _Nodes;
};

}
