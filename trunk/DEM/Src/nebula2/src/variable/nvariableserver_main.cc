//------------------------------------------------------------------------------
//  nvariableserver_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "kernel/nkernelserver.h"
#include "variable/nvariableserver.h"
#include "variable/nvariablecontext.h"

nVariableServer* nVariableServer::Singleton = 0;

//------------------------------------------------------------------------------
/**
*/
nVariableServer::nVariableServer() :
    registry(64, 64)
{
    n_assert(0 == Singleton);
    Singleton = this;
}

//------------------------------------------------------------------------------
/**
*/
nVariableServer::~nVariableServer()
{
    n_assert(Singleton);
    Singleton = 0;
}

//------------------------------------------------------------------------------
/**
    Declare a new variable name with it's associated fourcc code. Returns
    the handle of the variable.

    NOTE: variables which are not declared are declared on the fly
    by GetVariableHandleByName() or GetVariableHandleByFourCC(), however,
    those functions will let either the name or the fourcc code of the
    variable in an invalid state. Only variables which are properly
    declared before used will get a valid name AND fourcc code.

    @param  varName     name of a new variable
    @param  varFourCC   associated fourcc code of variable
    @return             variable handle of new variable
*/
nVariable::Handle
nVariableServer::DeclareVariable(const char* varName, nFourCC varFourCC)
{
    n_assert(varName);
    n_assert(0 != varFourCC);

    // check if the variable already exists, either by name or by fourcc
    nVariable::Handle nameHandle = this->FindVariableHandleByName(varName);
    nVariable::Handle fourccHandle = this->FindVariableHandleByFourCC(varFourCC);
    if ((nameHandle != nVariable::InvalidHandle) || (fourccHandle != nVariable::InvalidHandle))
    {
        // variable is already declared, make sure that the declaration is consistent
        if (nameHandle != fourccHandle)
        {
            char buf[5];
            n_error("Inconsistent variable declaration: '%s'/'%s'!\n"
                    "Either the name or the fourcc code is already used",
                    varName, this->FourCCToString(varFourCC, buf, sizeof(buf)));
            return nVariable::InvalidHandle;
        }
        return nameHandle;
    }
    else
    {
        // a new variable declaration
        VariableDeclaration newDecl(varName, varFourCC);
        this->registry.Append(newDecl);
        return this->registry.Size() - 1;
    }
}

//------------------------------------------------------------------------------
/**
    Returns number of variable declarations.

    @return     number of variable declarations
*/
int
nVariableServer::GetNumVariables() const
{
    return this->registry.Size();
}

//------------------------------------------------------------------------------
/**
    Returns variable declaration attributes at given index. The index
    is NOT the same as the variable handle.

    @param  index       [in] variable index
    @param  varName     [out] variable name
    @param  varFourCC   [out] variable fourcc code
*/
void
nVariableServer::GetVariableAt(int index, const char*& varName, nFourCC& varFourCC)
{
    varName   = this->registry[index].GetName();
    varFourCC = this->registry[index].GetFourCC();
}

//------------------------------------------------------------------------------
/**
    Find a variable handle by variable name. The variable must have been
    declared before. Otherwise returns nVariable::InvalidHandle.

    @param  varName     name of a variable
    @return             variable handle, or nVariable::InvalidHandle
*/
nVariable::Handle
nVariableServer::FindVariableHandleByName(const char* varName) const
{
    n_assert(varName);

    int i;
    int num = this->registry.Size();
    for (i = 0; i < num; i++)
    {
        if (strcmp(this->registry[i].GetName(), varName) == 0)
        {
            return i;
        }
    }
    // fallthrough: not found
    return nVariable::InvalidHandle;
}

//------------------------------------------------------------------------------
/**
    Find a variable handle by the variable's fourcc code. The variable must have been
    declared before. Otherwise returns nVariable::InvalidHandle.

    @param  varFourCC   fourcc code of a variable
    @return             variable handle, or nVariable::InvalidHandle
*/
nVariable::Handle
nVariableServer::FindVariableHandleByFourCC(nFourCC varFourCC) const
{
    n_assert(0 != varFourCC);

    int i;
    int num = this->registry.Size();
    for (i = 0; i < num; i++)
    {
        if (this->registry[i].GetFourCC() == varFourCC)
        {
            return i;
        }
    }
    // fallthrough: not found
    return nVariable::InvalidHandle;
}

//------------------------------------------------------------------------------
/**
    Get a variable handle by name. If variable doesn't exist yet, declare a
    new variable with valid name, but invalid fourcc.
*/
nVariable::Handle
nVariableServer::GetVariableHandleByName(const char* varName)
{
    n_assert(varName);
    nVariable::Handle varHandle = this->FindVariableHandleByName(varName);
    if (nVariable::InvalidHandle != varHandle)
    {
        return varHandle;
    }
    else
    {
        // a new variable declaration
        VariableDeclaration newDecl(varName);
        this->registry.Append(newDecl);
        return this->registry.Size() - 1;
    }
}

//------------------------------------------------------------------------------
/**
    Get a variable handle by fourcc. If variable doesn't exist yet, declare a
    new variable with valid fourcc, but invalid name.
*/
nVariable::Handle
nVariableServer::GetVariableHandleByFourCC(nFourCC fourcc)
{
    n_assert(0 != fourcc);
    nVariable::Handle varHandle = this->FindVariableHandleByFourCC(fourcc);
    if (nVariable::InvalidHandle != varHandle)
    {
        return varHandle;
    }
    else
    {
        // a new variable declaration
        VariableDeclaration newDecl(fourcc);
        this->registry.Append(newDecl);
        return this->registry.Size() - 1;
    }
}

//------------------------------------------------------------------------------
/**
    Return the name for a variable handle.

    @param  h       a variable handle
    @return         the variable's name
*/
const char*
nVariableServer::GetVariableName(nVariable::Handle h)
{
    n_assert(nVariable::InvalidHandle != h);
    return this->registry[h].GetName();
}

//------------------------------------------------------------------------------
/**
    Return the fourcc code for a variable handle.

    @param  h       a variable handle
    @return         the variable's fourcc code
*/
nFourCC
nVariableServer::GetVariableFourCC(nVariable::Handle h)
{
    n_assert(nVariable::InvalidHandle != h);
    return this->registry[h].GetFourCC();
}


//------------------------------------------------------------------------------
/**
    Get float value of a global variable from the global variable
    context.
*/
float
nVariableServer::GetFloatVariable(nVariable::Handle varHandle) const
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        return var->GetFloat();
    }
    else
    {
        const char* variableName = this->Instance()->GetVariableName(varHandle);
        n_error("Variable  %s does not exist in global variable context!", variableName);
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
    Get vector4 value of a global variable from the global variable
    context.
*/
const vector4&
nVariableServer::GetVectorVariable(nVariable::Handle varHandle) const
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        return var->GetVector4();
    }
    else
    {
        const char* variableName = this->Instance()->GetVariableName(varHandle);
        n_error("Variable '%s' does not exist in global variable context!", variableName);
        static vector4 zero;
        return zero;
    }
}

//------------------------------------------------------------------------------
/**
    Get integer value of a global variable from the global variable
    context.
*/
int
nVariableServer::GetIntVariable(nVariable::Handle varHandle) const
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        return var->GetInt();
    }
    else
    {
        const char* variableName = this->Instance()->GetVariableName(varHandle);
        n_error("Variable '%s' does not exist in global variable context!", variableName);
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
    Get vector4 value of a global variable from the global variable
    context.
*/
const char*
nVariableServer::GetStringVariable(nVariable::Handle varHandle) const
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        return var->GetString();
    }
    else
    {
        const char* variableName = this->Instance()->GetVariableName(varHandle);
        n_error("Variable '%s' does not exist in global variable context!", variableName);
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
    Sets float value of a global variable in the global variable
    context.
*/
void
nVariableServer::SetFloatVariable(nVariable::Handle varHandle, float f)
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        var->SetFloat(f);
    }
    else
    {
        nVariable newVar(varHandle, f);
        this->globalVariableContext.AddVariable(newVar);
    }
}

//------------------------------------------------------------------------------
/**
    Sets integer value of a global variable in the global variable
    context.
*/
void
nVariableServer::SetIntVariable(nVariable::Handle varHandle, int i)
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        var->SetInt(i);
    }
    else
    {
        nVariable newVar(varHandle, i);
        this->globalVariableContext.AddVariable(newVar);
    }
}

//------------------------------------------------------------------------------
/**
    Sets vector4 value of a global variable in the global variable
    context.
*/
void
nVariableServer::SetVectorVariable(nVariable::Handle varHandle, const vector4& v)
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        var->SetVector4(v);
    }
    else
    {
        nVariable newVar(varHandle, v);
        this->globalVariableContext.AddVariable(newVar);
    }
}

//------------------------------------------------------------------------------
/**
    Sets string value of a global variable in the global variable
    context.
*/
void
nVariableServer::SetStringVariable(nVariable::Handle varHandle, const char* s)
{
    nVariable* var = this->globalVariableContext.GetVariable(varHandle);
    if (var)
    {
        var->SetString(s);
    }
    else
    {
        nVariable newVar(varHandle, s);
        this->globalVariableContext.AddVariable(newVar);
    }
}

