#ifndef N_CHARJOINT_H
#define N_CHARJOINT_H
//------------------------------------------------------------------------------
/**
    @class nCharJoint
    @ingroup Character

    @brief A joint in a character skeleton.

     - 06-Feb-03   floh    fixed for Nebula2

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "mathlib/vector.h"
#include "mathlib/matrix.h"
#include "mathlib/quaternion.h"
#include "util/nstring.h"

//------------------------------------------------------------------------------
class nCharJoint
{
public:

	nCharJoint();

	void SetParentJointIndex(int index) { parentJointIndex = index; }
    int GetParentJointIndex() const { return parentJointIndex; }
	void SetParentJoint(nCharJoint* p) { parentJoint = p; }
    nCharJoint* GetParentJoint() const { return parentJoint; }

	void SetPose(const vector3& t, const quaternion& q, const vector3& s);
	const vector3& GetPoseTranslate() const { return poseTranslate; }
	const quaternion& GetPoseRotate() const { return poseRotate; }
	const vector3& GetPoseScale() const { return poseScale; }
	const matrix44& GetPoseMatrix() const { return poseMatrix; } // Bind pose matrix flattened into the model space
	const matrix44& GetInvPoseMatrix() const { return invPoseMatrix; }

	void SetTranslate(const vector3& t) { translate = t; matrixDirty = true; }
	const vector3& GetTranslate() const { return translate; }
	void SetRotate(const quaternion& q) { rotate = q; matrixDirty = true; }
	const quaternion& GetRotate() const { return rotate; }
	void SetScale(const vector3& s) { scale = s; matrixDirty = true; }
	const vector3& GetScale() const { return scale; }
	void SetLocalMatrix(const matrix44& m) { localMatrix = m; matrixDirty = false; } // Local joint's tfm
	const matrix44& GetLocalMatrix() const { return localMatrix; }

	void SetName(const nString& NewName) { name = NewName; }
	const nString& GetName() const { return name; }

	void Evaluate();

	// Evaluated values
	void SetMatrix(const matrix44& m) { worldMatrix = m; lockMatrix = true; } // Sets model space matrix, doesn't mul on parent
	const matrix44& GetMatrix() const { return worldMatrix; } // Model space matrix
	const matrix44& GetSkinMatrix44() const { return skinMatrix44; } // Multiplied by inverse bind pose
 
	void ClearUptodateFlag() { isUptodate = false; }
	bool IsUptodate() const { return isUptodate; }

private:

    vector3 poseTranslate;
    quaternion poseRotate;
    vector3 poseScale;

    vector3 translate;
    quaternion rotate;
    vector3 scale;

    matrix44 poseMatrix;
    matrix44 invPoseMatrix;

    matrix44 localMatrix;
    matrix44 worldMatrix;
    matrix44 skinMatrix44;
    int parentJointIndex;
    nCharJoint* parentJoint;
    bool matrixDirty;
    bool lockMatrix;
    bool isUptodate;

    nString name;
};

inline nCharJoint::nCharJoint() :
	parentJoint(0),
	parentJointIndex(-1),
	poseScale(1.0f, 1.0f, 1.0f),
	scale(1.0f, 1.0f, 1.0f),
	matrixDirty(false),
	lockMatrix(false),
	isUptodate(false)
{
}
//---------------------------------------------------------------------

inline void nCharJoint::SetPose(const vector3& t, const quaternion& q, const vector3& s)
{
	poseTranslate = t;
	poseRotate = q;
	poseScale = s;

	poseMatrix.ident();
	poseMatrix.scale(poseScale);
	poseMatrix.mult_simple(matrix44(poseRotate));
	poseMatrix.translate(poseTranslate);

	// set the initial matrix so that it undoes the pose matrix
	localMatrix = poseMatrix;
	worldMatrix = poseMatrix;

	// global pose matrix and compute global inverse pose matrix
	if (parentJoint) poseMatrix.mult_simple(parentJoint->poseMatrix);
	invPoseMatrix = poseMatrix;
	invPoseMatrix.invert_simple();
}
//---------------------------------------------------------------------

// This computes the skinning matrix from the pose matrix, the translation, the
// rotation and the scale of the joint. The parent joint must already be uptodate!
inline void nCharJoint::Evaluate()
{
	if (isUptodate) return;

	if (!lockMatrix)
	{
		if (matrixDirty)
		{
			rotate.normalize();
			localMatrix.set(scale.x,	0.f,		0.f,		0.f,
							0.f,		scale.y,	0.f,		0.f,
							0.f,		0.f,		scale.z,	0.f,
							0.f,		0.f,		0.f,		1.f);
			localMatrix.mult_simple(matrix44(rotate));
			localMatrix.translate(translate);
			matrixDirty = false;
		}

		worldMatrix = localMatrix;

		if (parentJoint)
		{
			if (!parentJoint->IsUptodate()) parentJoint->Evaluate();
			worldMatrix.mult_simple(parentJoint->worldMatrix); //!!!can optimize - write result to m! (binary op or smth)
		}
	}

	//!!!can optimize - write result to m! (binary op or smth)
	skinMatrix44 = invPoseMatrix;
	skinMatrix44.mult_simple(worldMatrix);

	isUptodate = true;
}
//---------------------------------------------------------------------

#endif
