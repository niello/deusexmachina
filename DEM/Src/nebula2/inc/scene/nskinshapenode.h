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

namespace Data
{
	class CBinaryReader;
}

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
    const nCharSkeleton*	extCharSkeleton;
    nArray<Fragment>		fragmentArray;

public:

	nSkinShapeNode(): extCharSkeleton(NULL) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool ApplyGeometry(nSceneServer* sceneServer) { return true; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
	virtual int GetMeshUsage() const { return nMesh2::WriteOnce | nMesh2::NeedsVertexShader; }

	void BeginFragments(int num) { n_assert(num > 0); fragmentArray.SetFixedSize(num); }
	void SetFragGroupIndex(int fragIndex, int MeshGroupIdx) { fragmentArray[fragIndex].MeshGroupIdx = MeshGroupIdx; }
	int GetFragGroupIndex(int fragIndex) const { return fragmentArray[fragIndex].MeshGroupIdx; }
	void BeginJointPalette(int fragIndex, int numJoints) { fragmentArray[fragIndex].JointPalette.SetFixedSize(numJoints); }
	void SetJointIndex(int fragIndex, int paletteIndex, int jointIndex) { fragmentArray[fragIndex].JointPalette[paletteIndex] = jointIndex; }
	void EndJointPalette(int fragIndex) {}
	void EndFragments() {}
	int GetNumFragments() const { return fragmentArray.Size(); }
	int GetJointPaletteSize(int fragIndex) const { return fragmentArray[fragIndex].JointPalette.Size(); }
	int GetJointIndex(int fragIndex, int paletteIndex) const { return fragmentArray[fragIndex].JointPalette[paletteIndex]; }

	void SetSkinAnimator(const char* path) { n_assert(path); SkinAnimatorName = path; }
	const char* GetSkinAnimator() const { return SkinAnimatorName.Get(); }
	void SetCharSkeleton(const nCharSkeleton* charSkeleton) { n_assert(charSkeleton); extCharSkeleton = charSkeleton; }
	const nCharSkeleton* GetCharSkeleton() const { return extCharSkeleton; }
};

#endif
