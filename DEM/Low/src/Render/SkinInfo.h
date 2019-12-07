#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>
#include <Math/Matrix44.h>

// Shared bind pose data for skinning. Maps inverse skeleton-root-related bind
// pose transforms to skeleton bones.

namespace Render
{

struct CBoneInfo
{
	CStrID	ID;
	UPTR	ParentIndex;
};

class CSkinInfo: public Resources::CResourceObject
{
	FACTORY_CLASS_DECL;

protected:

	matrix44*              pInvBindPose;
	std::vector<CBoneInfo> Bones;
	//???root and terminal node indices?

public:

	CSkinInfo(): pInvBindPose(nullptr) {}
	virtual ~CSkinInfo() { Destroy(); }

	bool				Create(UPTR BoneCount);
	void				Destroy();

	virtual bool		IsResourceValid() const { return !!pInvBindPose; }

	matrix44*			GetInvBindPoseData() { return pInvBindPose; }
	const matrix44&		GetInvBindPose(UPTR BoneIndex) const { return pInvBindPose[BoneIndex]; }
	CBoneInfo&			GetBoneInfoEditable(UPTR BoneIndex) { return Bones[BoneIndex]; }
	const CBoneInfo&	GetBoneInfo(UPTR BoneIndex) const { return Bones[BoneIndex]; }
	UPTR				GetBoneCount() const { return Bones.size(); }
};

typedef Ptr<CSkinInfo> PSkinInfo;

}
