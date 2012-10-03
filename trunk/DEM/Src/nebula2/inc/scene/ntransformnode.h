#ifndef N_TRANSFORMNODE_H
#define N_TRANSFORMNODE_H
//------------------------------------------------------------------------------
/**
    @class nTransformNode
    @ingroup Scene

    @brief A transform node groups its child nodes and defines position,
    orientation and scale of a scene node. Transformation hierarchies
    can be created using the Nebula object name space hierarchy.

    Note that conversions through the GetXXX() method generally don't work.
    It is not possible to do a SetMatrix() and get the resulting
    orientation as quaternion with GetQuat().
    Similarly GetQuat() does NOT return the orientation set by
    SetEuler().

    See also @ref N2ScriptInterface_ntransformnode

    (C) 2002 RadonLabs GmbH
*/
#include "scene/nscenenode.h"
#include "mathlib/transform44.h"

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nTransformNode : public nSceneNode
{
public:
    /// constructor
	nTransformNode(): transformFlags(Active) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// called by nSceneServer when object is attached to scene
    virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// indicate the scene server that this node provides transformation
    virtual bool HasTransform() const;
    /// update transform and render into scene server
    virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix);

    /// lock to viewer position
    void SetLockViewer(bool b);
    /// locked to viewer position?
    bool GetLockViewer() const;
    /// lock to current matrix (never recompute)
    void SetLocked(bool b);
    /// currently locked?
    bool GetLocked() const;
    /// set active flag
    void SetActive(bool b);
    /// get active flag
    bool GetActive() const;
    /// set position in parent space
    void SetPosition(const vector3& p);
    /// get position in parent space (not valid if SetMatrix() was used)
    const vector3& GetPosition() const;
    /// set orientation as euler angles (rotation order is x -> y -> z, unit is RAD)
    void SetEuler(const vector3& e);
    /// get orientation as euler angles in parent space
    const vector3& GetEuler() const;
    /// set orientation as quaternion in parent space
    void SetQuat(const quaternion& q);
    /// get orientation as quaternion in parent space
    const quaternion& GetQuat() const;
    /// set scale in parent space
    void SetScale(const vector3& s);
    /// get scale in parent space
    const vector3& GetScale() const;
    /// set the optional rotate pivot
    void SetRotatePivot(const vector3& p);
    /// get the optional rotate pivot
    const vector3& GetRotatePivot() const;
    /// set the optional scale pivot
    void SetScalePivot(const vector3& p);
    /// get the optional scale pivot
    const vector3& GetScalePivot() const;
    /// set transform matrix (overrides position, euler, quaternion and scale)
    void SetTransform(const matrix44& m);
    /// get current transform matrix
    const matrix44& GetTransform();
    /// return true if scale pivot has been set
    bool HasScalePivot() const;
    /// return true if rotate pivot has been set
    bool HasRotatePivot() const;

protected:
    /// set a flags
    void SetFlags(ushort mask);
    /// unset a flags
    void UnsetFlags(ushort mask);
    /// check a flag mask
    bool CheckFlags(ushort mask) const;

    enum
    {
        Active = (1<<0),        // active/inactive
        LockViewer = (1<<1),    // locked to viewer position
    };

protected:
    transform44 tform;
    ushort transformFlags;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetFlags(ushort mask)
{
    this->transformFlags |= mask;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::UnsetFlags(ushort mask)
{
    this->transformFlags &= ~mask;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::CheckFlags(ushort mask) const
{
    return ((this->transformFlags & mask) == mask);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetLockViewer(bool b)
{
    if (b) this->SetFlags(LockViewer);
    else   this->UnsetFlags(LockViewer);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::GetLockViewer() const
{
    return this->CheckFlags(LockViewer);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetLocked(bool b)
{
    this->tform.setlocked(b);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::GetLocked() const
{
    return this->tform.islocked();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetActive(bool b)
{
    if (b) this->SetFlags(Active);
    else   this->UnsetFlags(Active);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::GetActive() const
{
    return this->CheckFlags(Active);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetPosition(const vector3& p)
{
    this->tform.settranslation(p);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTransformNode::GetPosition() const
{
    return this->tform.gettranslation();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetEuler(const vector3& e)
{
    this->tform.seteulerrotation(e);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTransformNode::GetEuler() const
{
    return this->tform.geteulerrotation();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetQuat(const quaternion& q)
{
    this->tform.setquatrotation(q);
}

//------------------------------------------------------------------------------
/**
*/
inline
const quaternion&
nTransformNode::GetQuat() const
{
    return this->tform.getquatrotation();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetScale(const vector3& s)
{
    this->tform.setscale(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTransformNode::GetScale() const
{
    return this->tform.getscale();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetRotatePivot(const vector3& p)
{
    this->tform.setrotatepivot(p, false); // default, do not balance this pivot transformation
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTransformNode::GetRotatePivot() const
{
    return this->tform.getrotatepivot();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTransformNode::SetScalePivot(const vector3& p)
{
    this->tform.setscalepivot(p);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTransformNode::GetScalePivot() const
{
    return this->tform.getscalepivot();
}

//------------------------------------------------------------------------------
/**
    Directly set the transformation matrix. Note that this will also set
    the locked flag to prevent evaluation of the resulting matrix from
    position, rotation and scale.
*/
inline
void
nTransformNode::SetTransform(const matrix44& m)
{
    this->tform.setmatrix(m);
    this->SetLocked(true);
}

//------------------------------------------------------------------------------
/**
    Return the current transformation. This is either computed from
    position, rotation and scale, or the matrix set with SetTransform()
    is returned.
*/
inline
const matrix44&
nTransformNode::GetTransform()
{
    return this->tform.getmatrix();
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::HasScalePivot() const
{
    return this->tform.hasscalepivot();
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTransformNode::HasRotatePivot() const
{
    return this->tform.hasrotatepivot();
}

//------------------------------------------------------------------------------
#endif
