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

class nTransformNode : public nSceneNode
{
protected:

	enum
	{
		Active = (1<<0),        // active/inactive
		LockViewer = (1<<1),    // locked to viewer position
	};

	transform44	tform;
	ushort		TfmFlags;

	void SetFlags(ushort mask) { TfmFlags |= mask; }
	void UnsetFlags(ushort mask) { TfmFlags &= ~mask; }
	bool CheckFlags(ushort mask) const { return ((TfmFlags & mask) == mask); }

public:

	nTransformNode(): TfmFlags(Active) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
	virtual bool HasTransform() const { return true; }
	virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix);

	void SetLockViewer(bool b);
	bool GetLockViewer() const { return CheckFlags(LockViewer); }
	void SetLocked(bool b) { tform.setlocked(b); } //don't recompute matrix
	bool GetLocked() const { return tform.islocked(); }
	void SetActive(bool b);
	bool GetActive() const { return CheckFlags(Active); }
	void SetPosition(const vector3& p) { tform.settranslation(p); }
	const vector3& GetPosition() const { return tform.gettranslation(); }
	void SetEuler(const vector3& e) { tform.seteulerrotation(e); }
	const vector3& GetEuler() const { return tform.geteulerrotation(); }
	void SetQuat(const quaternion& q) { tform.setquatrotation(q); }
	const quaternion& GetQuat() const { return tform.getquatrotation(); }
	void SetScale(const vector3& s) { tform.setscale(s); }
	const vector3& GetScale() const { return tform.getscale(); }
	void SetRotatePivot(const vector3& p) { tform.setrotatepivot(p, false); } // default, do not balance this pivot transformation
	const vector3& GetRotatePivot() const { return tform.getrotatepivot(); }
	bool HasRotatePivot() const { return tform.hasrotatepivot(); }
	void SetScalePivot(const vector3& p) { tform.setscalepivot(p); }
	const vector3& GetScalePivot() const { return tform.getscalepivot(); }
	bool HasScalePivot() const { return tform.hasscalepivot(); }
	void SetTransform(const matrix44& m) { tform.setmatrix(m); SetLocked(true); }
	const matrix44& GetTransform() { return tform.getmatrix(); }
};

inline void nTransformNode::SetLockViewer(bool b)
{
	if (b) SetFlags(LockViewer);
	else UnsetFlags(LockViewer);
}
//---------------------------------------------------------------------

inline void nTransformNode::SetActive(bool b)
{
	if (b) SetFlags(Active);
	else UnsetFlags(Active);
}
//---------------------------------------------------------------------

#endif
