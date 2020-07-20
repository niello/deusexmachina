#pragma once
#include <Data/RefCounted.h>
#include <Data/StringID.h>

// Mapping from transformation source (SRT array or animation clip track array) to
// actual scene nodes, relative to arbitrary root node. Used for output into CSkeleton.
// Refcounted because static poses recorded from animation clips share the same mapping
// and maybe in the future clips with identical mappings will be able to share it too.

namespace DEM::Anim
{
using PNodeMapping = Ptr<class CNodeMapping>;
class IPoseOutput;

class CNodeMapping : public Data::CRefCounted
{
public:

	constexpr static U16 NoParentIndex = std::numeric_limits<U16>().max();

	struct CNodeInfo
	{
		CStrID ID;
		U16    ParentIndex = NoParentIndex;
	};

	CNodeMapping(std::vector<CNodeInfo>&& Map);

	const UPTR       GetNodeCount() const { return _Map.size(); }
	const CNodeInfo& GetNodeInfo(UPTR Index) const { return _Map[Index]; }

	void             Bind(IPoseOutput& Output, std::vector<U16>& OutPorts) const;

protected:

	std::vector<CNodeInfo> _Map;
};

}
