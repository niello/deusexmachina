#pragma once
#ifndef __DEM_L1_RENDER_SKIN_INFO_H__
#define __DEM_L1_RENDER_SKIN_INFO_H__

#include <Resources/ResourceObject.h>
#include <Math/Matrix44.h>

// Shared bind pose data for skinning. Maps inverse skeleton-root-related bind
// pose transforms to skeleton bones.

namespace Render
{

class CSkinInfo: public Resources::CResourceObject
{
	__DeclareClass(CSkinInfo);

protected:

	//!!!allocate aligned!
	matrix44*	pInvBindPose;
	//!!!array of bone IDs (root-relative path)!
	//???root and terminal node indices?

public:

	CSkinInfo(): pInvBindPose(NULL) {}
	virtual ~CSkinInfo() { Destroy(); }

	bool					Create(const int& InitData);
	void					Destroy();

	virtual bool			IsResourceValid() const { return !!pInvBindPose; }

	//const matrix44&	GetInvBindPose(UPTR BoneIndex);
};

typedef Ptr<CSkinInfo> PSkinInfo;

}

#endif
