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
	void SetLocalMatrix(const matrix44& m) { localScaledMatrix = m; matrixDirty = false; } // Local joint's tfm
	const matrix44& GetLocalMatrix() const { return localScaledMatrix; }
 
	void SetVariationScale(const vector3& s) { variationScale = s; matrixDirty = true; }
	const vector3& GetVariationScale() const { return variationScale; }

	void SetName(const nString& NewName) { name = NewName; }
	const nString& GetName() const { return name; }

	void Evaluate();

	// Evaluated values
	void SetMatrix(const matrix44& m) { worldScaledMatrix = m; lockMatrix = true; } // Sets model space matrix, doesn't mul on parent
	const matrix44& GetMatrix() const { return worldScaledMatrix; } // Model space matrix
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
    vector3 variationScale;

    matrix44 poseMatrix;
    matrix44 invPoseMatrix;

    matrix44 localUnscaledMatrix;
    matrix44 localScaledMatrix;
    matrix44 worldUnscaledMatrix;
    matrix44 worldScaledMatrix;
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
	variationScale(1.0f, 1.0f, 1.0f),
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
	localScaledMatrix = poseMatrix;
	worldScaledMatrix = poseMatrix;

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

    // any changes in position/rotation/etc ?
    if (matrixDirty)
    {
        localScaledMatrix.ident();
        localUnscaledMatrix.ident();

        rotate.normalize();
        matrix44 rotateMatrix(rotate);

        // we need 2 local matrices, one scaled, one unscaled
        // the unscaled one is for our children, who need a parent matrix with uniform axis
        // the scaled one is for calculating the correct skin matrix
        localUnscaledMatrix.mult_simple(rotateMatrix);
        localUnscaledMatrix.translate(translate);

        localScaledMatrix.scale(scale);
        localScaledMatrix.scale(variationScale);
        localScaledMatrix.mult_simple(rotateMatrix);
        localScaledMatrix.translate(translate);

        matrixDirty = false;
    }

    if (!lockMatrix)
    {
        worldScaledMatrix = localScaledMatrix;
        worldUnscaledMatrix = localUnscaledMatrix;

        if (parentJoint)
        {
            if (!parentJoint->IsUptodate()) parentJoint->Evaluate();

            // joint translation is affected by parent scale while the actual axis are not
            vector3& trans = worldUnscaledMatrix.pos_component();
            trans.x *= parentJoint->scale.x * parentJoint->variationScale.x;
            trans.y *= parentJoint->scale.y * parentJoint->variationScale.y;
            trans.z *= parentJoint->scale.z * parentJoint->variationScale.z;

            trans = worldScaledMatrix.pos_component();
            trans.x *= parentJoint->scale.x * parentJoint->variationScale.x;
            trans.y *= parentJoint->scale.y * parentJoint->variationScale.y;
            trans.z *= parentJoint->scale.z * parentJoint->variationScale.z;

            // we calculate 2 world matrices
            // the unscaled one has uniform axis, which our children need to calculate their matrices
            // the scaled one is the one used to calculate the skin matrix (the applied scaling is the local,
            // parent scaling which influences the translation of the joint has been handled above)
            worldUnscaledMatrix.mult_simple(parentJoint->worldUnscaledMatrix);
            worldScaledMatrix.mult_simple(parentJoint->worldUnscaledMatrix);
        }
    }

    skinMatrix44 = invPoseMatrix * worldScaledMatrix;

	isUptodate = true;
}
//---------------------------------------------------------------------

#endif
