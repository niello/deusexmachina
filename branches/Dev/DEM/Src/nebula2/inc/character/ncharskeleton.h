#ifndef N_CHARSKELETON_H
#define N_CHARSKELETON_H

#include "character/ncharjoint.h"
#include "util/nfixedarray.h"

// Implements a character skeleton made of nCharJoint objects.
// (C) 2002 RadonLabs GmbH

class nCharSkeleton
{
private:

	nFixedArray<nCharJoint> Joints;

	void Copy(const nCharSkeleton& Other);

public:

	nCharSkeleton() {}
	nCharSkeleton(const nCharSkeleton& Other) { Copy(Other); }

	void		BeginJoints(int num) { n_assert(num > 0); Joints.SetSize(num); }
	void		SetJoint(int index, int parentIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name);
	void		EndJoints() {}
	int			GetNumJoints() const { return Joints.Size(); }
	nCharJoint&	GetJointAt(int index) const { return Joints[index]; }
	int			GetJointIndexByName(const nString& name) const;
	void		Clear() { Joints.SetSize(0); }
	void		Evaluate();

	void operator =(const nCharSkeleton& Other) { n_assert(&Other != this); Copy(Other); }
};

inline void nCharSkeleton::Evaluate()
{
	for (int i = 0; i < Joints.Size(); i++)
		Joints[i].ClearUptodateFlag();
	for (int i = 0; i < Joints.Size(); i++)
		Joints[i].Evaluate();
}
//---------------------------------------------------------------------

inline void nCharSkeleton::Copy(const nCharSkeleton& Other)
{
	Joints = Other.Joints;
	for (int i = 0; i < Joints.Size(); i++)
	{
		int ParentIdx = Joints[i].GetParentJointIndex();
		if (ParentIdx != -1) Joints[i].SetParentJoint(&Joints[ParentIdx]);
	}
}
//---------------------------------------------------------------------

inline void nCharSkeleton::SetJoint(int index, int parentIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name)
{
	nCharJoint& newJoint = Joints[index];
	newJoint.SetParentJointIndex(parentIndex);
	newJoint.SetParentJoint((parentIndex == -1) ? NULL : &Joints[parentIndex]);
	newJoint.SetPose(poseTranslate, poseRotate, poseScale);
	newJoint.SetName(name);
}
//---------------------------------------------------------------------

inline int nCharSkeleton::GetJointIndexByName(const nString& name) const
{
	for (int index = 0; index < Joints.Size(); index++)
		if (Joints[index].GetName() == name)
			return index;
	return -1;
}
//---------------------------------------------------------------------

#endif
