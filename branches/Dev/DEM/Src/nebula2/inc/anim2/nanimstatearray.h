#ifndef N_ANIMSTATEARRAY_H
#define N_ANIMSTATEARRAY_H
//------------------------------------------------------------------------------
/**
    @class nAnimStateArray
    @ingroup Anim2

    @brief An animation state array holds several mutually exclusive nAnimState
    objects and allows switching between them.

    (C) 2003 RadonLabs GmbH
*/
#include "anim2/nanimstateinfo.h"
#include "util/narray.h"

//------------------------------------------------------------------------------
class nAnimStateArray
{
public:
    /// constructor
    nAnimStateArray();
    /// destructor
    ~nAnimStateArray();
    /// begin adding animation states
    void Begin(int num);
    /// set a new animation state
    void SetState(int index, const nAnimStateInfo& state);
    /// finish adding animation states
    void End();
    /// get number of animation states
    int GetNumStates() const;
    /// get an animation state object at given index
    nAnimStateInfo& GetStateAt(int index) const;
    /// find state by name
    nAnimStateInfo* FindState(const nString& n) const;
    /// find a state index by name
    int FindStateIndex(const nString& n) const;

private:
    nArray<nAnimStateInfo> stateArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimStateArray::nAnimStateArray() :
    stateArray(0, 0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimStateArray::~nAnimStateArray()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Begin adding states to the animation state array.

    @param  num     number of animation states
*/
inline
void
nAnimStateArray::Begin(int num)
{
    this->stateArray.SetFixedSize(num);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateArray::SetState(int index, const nAnimStateInfo& state)
{
    this->stateArray[index] = state;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimStateArray::End()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAnimStateArray::GetNumStates() const
{
    return this->stateArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimStateInfo&
nAnimStateArray::GetStateAt(int index) const
{
    return this->stateArray[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
nAnimStateInfo*
nAnimStateArray::FindState(const nString& n) const
{
    int i;
    int num = this->stateArray.Size();
    for (i = 0; i < num; i++)
    {
        nAnimStateInfo& state = this->stateArray[i];
        if (state.GetName() == n)
        {
            return &state;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    Finds a state index by name. Returns -1 if state not found.
*/
inline
int
nAnimStateArray::FindStateIndex(const nString& n) const
{
    int i;
    int num = this->stateArray.Size();
    for (i = 0; i < num; i++)
    {
        nAnimStateInfo& state = this->stateArray[i];
        if (state.GetName() == n)
        {
            return i;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
#endif
