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
#include "character/ncharjointpalette.h"
#include "character/ncharskinrenderer.h"

class nCharSkeleton;
class nSkinAnimator;

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nSkinShapeNode : public nShapeNode
{
public:
    /// constructor
    nSkinShapeNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// perform pre-instance geometry setup
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// render per-instance geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// render debug information
    virtual void RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix);
    /// get the mesh usage flags required by this shape node
    virtual int GetMeshUsage() const;
    /// set the skin animator
    void SetSkinAnimator(const char* path);
    /// get the skin animator
    const char* GetSkinAnimator() const;
    /// set pointer to an uptodate character skeleton object (called exclusively by nSkinAnimator)
    void SetCharSkeleton(const nCharSkeleton* charSkeleton);
    /// get pointer to an uptodate character skeleton object
    const nCharSkeleton* GetCharSkeleton() const;
    /// begin defining mesh fragments
    void BeginFragments(int num);
    /// set mesh group index of a skin fragment
    void SetFragGroupIndex(int fragIndex, int meshGroupIndex);
    /// get mesh group index of a skin fragment
    int GetFragGroupIndex(int fragIndex) const;
    /// begin defining the joint palette of a fragment
    void BeginJointPalette(int fragIndex, int numJoints);
    /// add up to 8 joint indices to the fragments joint palette
    void SetJointIndices(int fragIndex, int paletteIndex, int ji0, int ji1, int ji2, int ji3, int ji4, int ji5, int ji6, int ji7);
    /// add a single joint index to the fragments joint palette
    void SetJointIndex(int fragIndex, int paletteIndex, int jointIndex);
    /// finish adding joints to the joint palette
    void EndJointPalette(int fragIndex);
    /// finish defining fragments
    void EndFragments();
    /// get number of fragments
    int GetNumFragments() const;
    /// get number of joints in a fragment's joint palette
    int GetJointPaletteSize(int fragIndex) const;
    /// get a joint index from a fragment's joint palette
    int GetJointIndex(int fragIndex, int paletteIndex) const;
    /// set a marker that this is a skin that is bound to a variation
    void SetChar3VariationFlag(bool value);
    /// get a marker if this is a skin that is bound to a variation
    bool GetChar3VariationFlag() const;

private:
    /// a private skin fragment class
    class Fragment
    {
    public:
        /// constructor
        Fragment();
        /// set mesh group index
        void SetMeshGroupIndex(int i);
        /// get mesh group index
        int GetMeshGroupIndex() const;
        /// get embedded joint palette
        nCharJointPalette& GetJointPalette();

    private:
        int meshGroupIndex;
        nCharJointPalette jointPalette;
    };

	nString SkinAnimatorName;
    const nCharSkeleton* extCharSkeleton;
    nArray<Fragment> fragmentArray;
    nCharSkinRenderer charSkinRenderer;
    bool isChar3AndBoundToVariation;
};

//------------------------------------------------------------------------------
/**
*/
inline
nSkinShapeNode::Fragment::Fragment() :
    meshGroupIndex(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkinShapeNode::Fragment::SetMeshGroupIndex(int i)
{
    this->meshGroupIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nSkinShapeNode::Fragment::GetMeshGroupIndex() const
{
    return this->meshGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharJointPalette&
nSkinShapeNode::Fragment::GetJointPalette()
{
    return this->jointPalette;
}


//------------------------------------------------------------------------------
/**
*/
inline
void
nSkinShapeNode::SetChar3VariationFlag(bool value)
{
    this->isChar3AndBoundToVariation = value;
};


//------------------------------------------------------------------------------
/**
*/
inline
bool
nSkinShapeNode::GetChar3VariationFlag() const
{
    return this->isChar3AndBoundToVariation;
};

//------------------------------------------------------------------------------
#endif
