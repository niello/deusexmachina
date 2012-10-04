#ifndef N_CHARSKELETON_H
#define N_CHARSKELETON_H
//------------------------------------------------------------------------------
/**
    @class nCharSkeleton
    @ingroup Character

    @brief Implements a character skeleton made of nCharJoint objects.

    (C) 2002 RadonLabs GmbH
*/

#include "character/ncharjoint.h"
#include "util/nfixedarray.h"

//------------------------------------------------------------------------------
class nCharSkeleton
{
public:
    /// constructor
    nCharSkeleton();
    /// destructor
    ~nCharSkeleton();
    /// copy constructor
    nCharSkeleton(const nCharSkeleton& src);
    /// assignment operator
    void operator=(const nCharSkeleton& src);
    /// clear content
    void Clear();
    /// begin adding joints
    void BeginJoints(int num);
    /// add a joint to the joint skeleton
    void SetJoint(int index, int parentIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name);
    /// finish adding joints
    void EndJoints();
    /// get number of joint
    int GetNumJoints() const;
    /// get joint by index
    nCharJoint& GetJointAt(int index) const;
    /// get joint index by name
    int GetJointIndexByName(const nString& name) const;
    /// evaluate all character joints
    void Evaluate();

private:
    /// update the parent joint pointers from their indexes
    void UpdateParentJointPointers();

    nFixedArray<nCharJoint> jointArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharSkeleton::Evaluate()
{
    // first pass: clear uptodate flag
    int i;
    int num = this->jointArray.Size();
    for (i = 0; i < num; i++)
    {
        this->jointArray[i].ClearUptodateFlag();
    }
    // seconds pass: evaluate joints
    for (i = 0; i < num; i++)
    {
        this->jointArray[i].Evaluate();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharSkeleton::nCharSkeleton()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Update the parent joint pointers in the joint array.
*/
inline
void
nCharSkeleton::UpdateParentJointPointers()
{
    int i;
    for (i = 0; i < this->jointArray.Size(); i++)
    {
        int parentJointIndex = this->jointArray[i].GetParentJointIndex();
        if (parentJointIndex != -1)
        {
            jointArray[i].SetParentJoint(&(this->jointArray[parentJointIndex]));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharSkeleton::nCharSkeleton(const nCharSkeleton& src)
{
    this->jointArray = src.jointArray;
    this->UpdateParentJointPointers();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharSkeleton::operator=(const nCharSkeleton& src)
{
    n_assert(&src != this);
    this->jointArray = src.jointArray;
    this->UpdateParentJointPointers();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharSkeleton::Clear()
{
    this->jointArray.SetSize(0);
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharSkeleton::~nCharSkeleton()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Start joint skeleton definition.

    @param  num     number of joints in skeleton
*/
inline
void
nCharSkeleton::BeginJoints(int num)
{
    n_assert(num > 0);
    this->jointArray.SetSize(num);
}

//------------------------------------------------------------------------------
/**
    Add a joint to the skeleton.
*/
inline
void
nCharSkeleton::SetJoint(int index, int parentIndex, const vector3& poseTranslate, const quaternion& poseRotate, const vector3& poseScale, const nString& name)
{
    nCharJoint newJoint;
    newJoint.SetParentJointIndex(parentIndex);
    if (-1 == parentIndex)
    {
        newJoint.SetParentJoint(0);
    }
    else
    {
        newJoint.SetParentJoint(&(this->jointArray[parentIndex]));
    }
    newJoint.SetPose(poseTranslate, poseRotate, poseScale);
    newJoint.SetName(name);
    this->jointArray[index] = newJoint;
}

//------------------------------------------------------------------------------
/**
    Finish defining joints.
*/
inline
void
nCharSkeleton::EndJoints()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharSkeleton::GetNumJoints() const
{
    return this->jointArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nCharJoint&
nCharSkeleton::GetJointAt(int index) const
{
    return this->jointArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharSkeleton::GetJointIndexByName(const nString& name) const
{
    int index;
    for (index = 0; index < this->jointArray.Size(); index++)
    {
        if (this->jointArray[index].GetName() == name)
        {
            return index;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
#endif
