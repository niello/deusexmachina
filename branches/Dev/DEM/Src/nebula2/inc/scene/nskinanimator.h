#ifndef N_SKINANIMATOR_H
#define N_SKINANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nSkinAnimator
    @ingroup Scene

    @brief Provide an animated joint skeleton for a nSkinShapeNode.

    On Animate() invocation, the nSkinAnimator will update its joint
    skeleton and invoke SetCharSkeleton() on the calling
    scene node (which must be a nSkinShapeNode) with a pointer
    to an uptodate nCharSkeleton object.

    See also @ref N2ScriptInterface_nskinanimator

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nscenenode.h"
#include "character/ncharskeleton.h"
#include "character/ncharacter2.h"
#include "anim2/nanimstateinfo.h"

class nAnimation;
class nAnimationServer;
class nCharacter2Set;

namespace Data
{
	class CBinaryReader;
}


class nAnimLoopType
{
public:

	enum Type
    {
        Loop,
        Clamp,
    };

    static nString ToString(nAnimLoopType::Type t);
    static nAnimLoopType::Type FromString(const nString& s);
};

class nSkinAnimator: public nSceneNode
{
public:

	nAnimLoopType::Type	LoopType;

    nSkinAnimator();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();
    /// called by app when new render context has been created for this object
    virtual void RenderContextCreated(nRenderContext* renderContext);
    /// called by app when render context is going to be released
    virtual void RenderContextDestroyed(nRenderContext* renderContext);
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);

    /// begin adding joints
    void BeginJoints(int numJoints);
    /// add a joint to the skeleton
    void SetJoint(int index, int parentJointIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name);
    /// finish adding joints
    void EndJoints();
    /// get number of joints in skeleton
    int GetNumJoints();
    /// get joint attributes
    void GetJoint(int index, int& parentJointIndex, vector3& poseTranslate, quaternion& poseRotate, vector3& poseScale, nString& name);
    /// set name of an animation resource
    void SetAnim(const nString& filename);
    /// get name of an animation resource
    const nString& GetAnim() const;

    /// begin adding clips
    void BeginClips(int numClips);
    /// add an animation clip
    void SetClip(int clipIndex, int animGroupIndex, const nString& clipName);
    /// finish adding clips
    void EndClips();
    /// get number of clips in the animation
    int GetNumClips() const;
    /// get clip at index
    const nAnimClip& GetClipAt(int clipIndex) const;
    /// get clip by name
    int GetClipIndexByName(const nString& name) const;
    /// get clip duration
    nTime GetClipDuration(int index) const;

    /// begin adding animation event tracks to a clip
    void BeginAnimEventTracks(int clipIndex, int numTracks);
    /// begin an event track to the current clip
    void BeginAnimEventTrack(int clipIndex, int trackIndex, const nString& name, int numEvents);
    /// set an animation event in a track
    void SetAnimEvent(int clipIndex, int trackIndex, int eventIndex, float time, const vector3& translate, const quaternion& rotate, const vector3& scale);
    /// end the current event track
    void EndAnimEventTrack(int clipIndex, int trackIndex);
    /// end adding animation event tracks to current clip
    void EndAnimEventTracks(int clipIndex);

    /// enable/disable animation
    void SetAnimEnabled(bool b);
    /// get animation enabled state
    bool IsAnimEnabled() const;

    /// set the index for the render context
    //void SetCharacterSetIndexHandle(int handle);
    /// get index of character set variable in render context
    int GetCharacterSetIndexHandle() const;
    ///
    int GetCharacterVarIndexHandle() const;
    ///
    int GetJointByName(const char* jointName);

	void				SetChannel(const char* name);
	const char*			GetChannel();

protected:

	nVariable::Handle	HChannel;
	nVariable::Handle	HChannelOffset;

    nCharacter2 character;          ///< blue print, one copy per render context will be created
    nRef<nAnimation> refAnim;       ///< pointer to loaded animation
    nString animName;               ///< name of the animation
    nArray<nAnimClip> clips;        ///< array of animation clips
    bool animEnabled;
    int characterVarIndex;
    int characterSetIndex;
    uint frameId;
    nClass* skinShapeNodeClass;

    bool LoadAnim();
    void UnloadAnim();
};

inline nString nAnimLoopType::ToString(nAnimLoopType::Type t)
{
    switch (t)
    {
        case Loop:  return nString("loop");
        case Clamp: return nString("clamp");
        default:
            n_error("nAnimLoopType::ToString(): invalid enum value!");
            return nString("");
    }
}
//---------------------------------------------------------------------

inline nAnimLoopType::Type nAnimLoopType::FromString(const nString& s)
{
    if (s == "loop") return Loop;
    else if (s == "clamp") return Clamp;
    else
    {
        n_error("nAnimLoopType::ToString(): invalid loop type '%s'\n", s.Get());
        return Clamp;
    }
}
//---------------------------------------------------------------------

inline
int
nSkinAnimator::GetCharacterVarIndexHandle() const
{
    return this->characterVarIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nSkinAnimator::GetCharacterSetIndexHandle() const
{
    return this->characterSetIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkinAnimator::SetAnimEnabled(bool b)
{
    this->animEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSkinAnimator::IsAnimEnabled() const
{
    return this->animEnabled;
}

//------------------------------------------------------------------------------
/**
    Begin configuring the joint skeleton.
*/
inline
void
nSkinAnimator::BeginJoints(int numJoints)
{
    this->character.GetSkeleton().BeginJoints(numJoints);
}

//------------------------------------------------------------------------------
/**
    Add a joint to the joint skeleton.
*/
inline
void
nSkinAnimator::SetJoint(int jointIndex, int parentJointIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name)
{
    this->character.GetSkeleton().SetJoint(jointIndex, parentJointIndex, poseTranslate, poseRotate, poseScale, name);
}

//------------------------------------------------------------------------------
/**
    Finish adding joints to the joint skeleton.
*/
inline
void
nSkinAnimator::EndJoints()
{
    this->character.GetSkeleton().EndJoints();
}

//------------------------------------------------------------------------------
/**
    Get number of joints in joint skeleton.
*/
inline
int
nSkinAnimator::GetNumJoints()
{
    return this->character.GetSkeleton().GetNumJoints();
}

//------------------------------------------------------------------------------
/**
    Get joint attributes.
*/
inline
void
nSkinAnimator::GetJoint(int index, int& parentJointIndex, vector3& poseTranslate, quaternion& poseRotate, vector3& poseScale, nString& name)
{
    nCharJoint& joint = this->character.GetSkeleton().GetJointAt(index);
    parentJointIndex = joint.GetParentJointIndex();
    poseTranslate = joint.GetPoseTranslate();
    poseRotate    = joint.GetPoseRotate();
    poseScale     = joint.GetPoseScale();
    name          = joint.GetName();
}

//------------------------------------------------------------------------------
/**
    Set name of animation resource file.
*/
inline
void
nSkinAnimator::SetAnim(const nString& name)
{
    this->UnloadAnim();
    this->animName = name;
}

//------------------------------------------------------------------------------
/**
    Get name of animation resource file.
*/
inline
const nString&
nSkinAnimator::GetAnim() const
{
    return this->animName;
}

//------------------------------------------------------------------------------
#endif
