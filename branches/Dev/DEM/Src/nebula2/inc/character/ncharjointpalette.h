#ifndef N_CHARJOINTPALETTE_H
#define N_CHARJOINTPALETTE_H
//------------------------------------------------------------------------------
/**
    @class nCharJointPalette
    @ingroup Character

    @brief A joint palette defines a subset of a character skeleton to deal
    with hardware which can process only a limited number of joints for
    skinned mesh rendering.

    (C) 2003 RadonLabs GmbH
*/

//------------------------------------------------------------------------------
class nCharJointPalette
{
public:
    /// constructor
    nCharJointPalette();
    /// begin defining joints
    void BeginJoints(int num);
    /// set a joint index
    void SetJointIndex(int paletteIndex, int jointIndex);
    /// finish defining joints
    void EndJoints();
    /// get number of joints in palette
    int GetNumJoints() const;
    /// get joint index at given palette index
    int GetJointIndexAt(int paletteIndex) const;

private:
    nArray<int> jointIndexArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nCharJointPalette::nCharJointPalette() :
    jointIndexArray(0, 0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharJointPalette::BeginJoints(int num)
{
    this->jointIndexArray.SetFixedSize(num);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharJointPalette::SetJointIndex(int paletteIndex, int jointIndex)
{
    this->jointIndexArray[paletteIndex] = jointIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCharJointPalette::EndJoints()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharJointPalette::GetNumJoints() const
{
    return this->jointIndexArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nCharJointPalette::GetJointIndexAt(int paletteIndex) const
{
    return this->jointIndexArray[paletteIndex];
}

//------------------------------------------------------------------------------
#endif
