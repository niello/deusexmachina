#ifndef N_VARIABLECONTEXT_H
#define N_VARIABLECONTEXT_H
//------------------------------------------------------------------------------
/**
    @class nVariableContext
    @ingroup NebulaVariableSystem

    A variable context is a container for nVariable objects. Variables
    are accessed by their global nVariableHandle.

    (C) 2002 RadonLabs GmbH
*/

#include "util/narray.h"
#include "variable/nvariable.h"

//------------------------------------------------------------------------------
class nVariableContext
{
public:
    /// constructor
    nVariableContext();
    /// destructor
    ~nVariableContext();
    /// clear the variable context
    void Clear();
    /// add a variable to the context
    void AddVariable(const nVariable& var);
    /// update existing variable or append new var
    void SetVariable(const nVariable& var);
    /// get a variable object by its handle
    nVariable* GetVariable(nVariable::Handle handle) const;
    /// get number of variables in context
    int GetNumVariables() const;
    /// get variable at index (this is NOT the variable handle!)
    nVariable& GetVariableAt(int index) const;

private:
    nArray<nVariable> varArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nVariableContext::nVariableContext() :
    varArray(4, 4)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariableContext::~nVariableContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariableContext::Clear()
{
    this->varArray.Clear();
}

//------------------------------------------------------------------------------
/**
    @param  h       the variable handle
    @return         pointer to variable object, or 0 if variable doesn't exist
                    in this context
*/
inline
nVariable*
nVariableContext::GetVariable(nVariable::Handle h) const
{
    int size = this->varArray.Size();
    int i;
    for (i = 0; i < size; i++)
    {
        if (this->varArray[i].GetHandle() == h)
        {
            return &(this->varArray[i]);
        }
    }
    // fallthrough: variable not found
    return 0;
}

//------------------------------------------------------------------------------
/**
    Add a new variable to the context. The variable MAY NOT YET EXIST
    in the context. Check this with GetVariable() first! No checks are
    made if a duplicate variable is added to the context!
*/
inline
void
nVariableContext::AddVariable(const nVariable& var)
{
    this->varArray.Append(var);
}

//------------------------------------------------------------------------------
/**
    Update value of existing variable, or create new if it doesn't exist.
*/
inline
void
nVariableContext::SetVariable(const nVariable& var)
{
    nVariable* varPtr = this->GetVariable(var.GetHandle());
    if (varPtr)
    {
        *varPtr = var;
    }
    else
    {
        this->AddVariable(var);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nVariableContext::GetNumVariables() const
{
    return this->varArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable&
nVariableContext::GetVariableAt(int index) const
{
    return this->varArray[index];
}

//------------------------------------------------------------------------------
#endif

