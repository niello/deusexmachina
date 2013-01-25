#ifndef N_SKINSHAPENODE_H
#define N_SKINSHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nSkinShapeNode
    @ingroup Scene

    @brief A smooth skinned shape node.

    Requires that a nSkinAnimator is attached as an animator which is
    called back by the skin shape before rendering to provide a valid
    joint palette.

    See also @ref N2ScriptInterface_nskinshapenode

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nshapenode.h"
#include "util/nstring.h"

class nCharSkeleton;
class nSkinAnimator;

class nSkinShapeNode: public nShapeNode
{
private:
 
	// A private skin fragment class, to support HW with limited joint count per pass
    struct Fragment
    {
		nArray<int>	JointPalette;
		int			MeshGroupIdx;

		Fragment(): MeshGroupIdx(0) {}
    };

	nString					SkinAnimatorName;
    const nCharSkeleton*	pSkeleton;
    nArray<Fragment>		Fragments;

public:

	nSkinShapeNode(): pSkeleton(NULL) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool ApplyGeometry(nSceneServer* sceneServer) { return true; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

	void SetSkinAnimator(const char* path) { n_assert(path); SkinAnimatorName = path; }
	const char* GetSkinAnimator() const { return SkinAnimatorName.Get(); }
	void SetCharSkeleton(const nCharSkeleton* charSkeleton) { n_assert(charSkeleton); pSkeleton = charSkeleton; }
	const nCharSkeleton* GetCharSkeleton() const { return pSkeleton; }
};

#endif
