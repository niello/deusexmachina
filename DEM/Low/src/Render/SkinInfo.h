#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>
#include <rtm/matrix3x4f.h>

// Shared bind pose data for skinning. Maps inverse skeleton-root-related bind
// pose transforms to skeleton bones.

namespace Render
{

struct CBoneInfo
{
	CStrID	ID;
	UPTR	ParentIndex;
};

class CSkinInfo: public DEM::Core::CObject
{
	FACTORY_CLASS_DECL;

public:

	// FIXME: public for loaders
	rtm::matrix3x4f*       pInvBindPose = nullptr;
	std::vector<CBoneInfo> Bones;
	//???root and terminal node indices?

	virtual ~CSkinInfo() override;

	bool				   Create(UPTR BoneCount);
	void				   Destroy();

	UPTR                   GetBoneMatchingLength(const CSkinInfo& Other) const;

	const rtm::matrix3x4f& GetInvBindPose(UPTR BoneIndex) const { return pInvBindPose[BoneIndex]; }
	const CBoneInfo&	   GetBoneInfo(UPTR BoneIndex) const { return Bones[BoneIndex]; }
	UPTR				   GetBoneCount() const { return Bones.size(); }
};

typedef Ptr<CSkinInfo> PSkinInfo;

}
