#ifndef N_CHARSKELETON_H
#define N_CHARSKELETON_H

#include "character/ncharjoint.h"
#include "util/nfixedarray.h"

//!!!TMP!
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>

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
	int			GetNumJoints() const { return Joints.Size(); }
	nCharJoint&	GetJointAt(int index) const { return Joints[index]; }
	int			GetJointIndexByName(const nString& name) const;
	void		Clear() { Joints.SetSize(0); }
	void		Evaluate();

	//!!!TMP! for conversion to DEM format
	Data::PParams CreateBoneHierarchyDesc(int BoneIdx)
	{
		Data::PParams Bone = n_new(Data::CParams);
		Data::PParams BoneAttr;

		if (BoneIdx > -1)
		{
			nCharJoint& Joint = Joints[BoneIdx];
			
			Data::PDataArray Attrs = n_new(Data::CDataArray);
			BoneAttr = n_new(Data::CParams);
			
			BoneAttr->Set(CStrID("Class"), nString("Bone"));
			//BoneAttr->Set(CStrID("PARENT_IDX"), Joint.GetParentJointIndex());
			BoneAttr->Set(CStrID("BoneIndex"), BoneIdx);

			if (Joint.GetParentJointIndex() < 0)
				BoneAttr->Set(CStrID("BoneType"), nString("Root"));

			if (Joint.GetPoseTranslate() != vector3::Zero)
				BoneAttr->Set(CStrID("PoseT"), vector4(Joint.GetPoseTranslate()));
			
			if (Joint.GetPoseRotate() != quaternion())
				BoneAttr->Set(	CStrID("PoseR"),
								vector4(Joint.GetPoseRotate().x,
										Joint.GetPoseRotate().y,
										Joint.GetPoseRotate().z,
										Joint.GetPoseRotate().w));
			
			if (Joint.GetPoseScale() != vector3(1.f, 1.f, 1.f))
				BoneAttr->Set(CStrID("PoseS"), vector4(Joint.GetPoseScale()));

			Attrs->Append(BoneAttr);
			
			Bone->Set(CStrID("Attrs"), Attrs);
		}

		Data::PParams Children;
		for (int i = 0; i < Joints.Size(); ++i)
			if (Joints[i].GetParentJointIndex() == BoneIdx)
			{
				if (!Children.isvalid())
				{
					Children = n_new(Data::CParams);
					Bone->Set(CStrID("Children"), Children);
				}

				Children->Set(CStrID(Joints[i].GetName().Get()), CreateBoneHierarchyDesc(i));
			}

		if (BoneAttr.isvalid() && !Children.isvalid())
			BoneAttr->Set(CStrID("BoneType"), nString("Term"));

		return Bone;
	}
	//==================================

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
