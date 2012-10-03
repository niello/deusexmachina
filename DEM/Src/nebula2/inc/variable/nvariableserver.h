#ifndef N_VARIABLESERVER_H
#define N_VARIABLESERVER_H
//------------------------------------------------------------------------------
/**
    @class nVariableServer
    @ingroup NebulaVariableSystem

    The variable server implements a registry for named, typed variables,
    and offers variable context object. The name registry guarantees a
    systemwide consistent mapping between variable names and handles.
    Variables are usually accessed by through their handle, as this is
    much faster.

    See also @ref N2ScriptInterface_nvariableserver

    (C) 2002 RadonLabs GmbH
*/

#include "kernel/nroot.h"
#include "util/narray.h"
#include "variable/nvariable.h"
#include "variable/nvariablecontext.h"
#include "mathlib/vector.h"

//------------------------------------------------------------------------------
class nVariableContext;

class nVariableServer: public nReferenced //nRoot
{
public:
    /// constructor
    nVariableServer();
    /// destructor
    virtual ~nVariableServer();
    /// return instance pointer
    static nVariableServer* Instance();

    /// declare a variable
    nVariable::Handle DeclareVariable(const char* varName, nFourCC fourcc);
    /// get number of variable declarations
    int GetNumVariables() const;
    /// get variable declaration attributes at given index
    void GetVariableAt(int index, const char*& varName, nFourCC& varFourCC);
    /// get a variable handle by name, if variable undeclared, create it
    nVariable::Handle GetVariableHandleByName(const char* varName);
    /// get a variable handle by fourcc, if variable undeclared, create it
    nVariable::Handle GetVariableHandleByFourCC(nFourCC fourcc);
    /// find variable handle by name
    nVariable::Handle FindVariableHandleByName(const char* varName) const;
    /// find variable handle by fourcc code
    nVariable::Handle FindVariableHandleByFourCC(nFourCC fourcc) const;
    /// get the name of a variable from its handle
    const char* GetVariableName(nVariable::Handle varHandle);
    /// get the fourcc code of a variable from its handle
    nFourCC GetVariableFourCC(nVariable::Handle varHandle);

    /// get the global variable context
    const nVariableContext& GetGlobalVariableContext() const;
    /// set a global variable
    void SetGlobalVariable(const nVariable& var);
    /// get a global variable by handle
    const nVariable* GetGlobalVariable(nVariable::Handle varHandle) const;
    /// get global variable by name
    const nVariable* GetGlobalVariable(const char* varName) const;
    /// return true if global variable exists by handle
    bool GlobalVariableExists(nVariable::Handle varHandle) const;
    /// return true if global variable exists by name
    bool GlobalVariableExists(const char* varName) const;
    /// get the float value of a global variable by handle
    float GetFloatVariable(nVariable::Handle varHandle) const;
    /// get float value of global variable by name
    float GetFloatVariable(const char* varName) const;
    /// get the vector4 value of a global variable by handle
    const vector4& GetVectorVariable(nVariable::Handle varHandle) const;
    /// get vector4 value of a global variable by name
    const vector4& GetVectorVariable(const char* varName) const;
    /// get the integer value of a global variable by handle
    int GetIntVariable(nVariable::Handle varHandle) const;
    /// get the integer value of a global by name
    int GetIntVariable(const char* varName) const;
    /// get the string value of a gloabl variable by handle
    const char* GetStringVariable(nVariable::Handle varHandle) const;
    /// get the string value of a global variable by name
    const char* GetStringVariable(const char* varName) const;
    /// set the float value of a global variable by handle
    void SetFloatVariable(nVariable::Handle varHandle, float v);
    /// set the float value of a global variable by name
    void SetFloatVariable(const char* varName, float v);
    /// set the vector4 value of a global variable by handle
    void SetVectorVariable(nVariable::Handle varHandle, const vector4& v);
    /// set the vector4 value of a global variable by name
    void SetVectorVariable(const char* varName, const vector4& v);
    /// set the integer value of a global variable by handle
    void SetIntVariable(nVariable::Handle varHandle, int i);
    /// set the integer value of a global variable by handle
    void SetIntVariable(const char* varName, int i);
    /// set the string value of a global variable by handle
    void SetStringVariable(nVariable::Handle varHandle, const char* s);
    /// set the string value of a global variable by name
    void SetStringVariable(const char* varName, const char* s);

    /// convert a string to a fourcc code
    static nFourCC StringToFourCC(const char* str);
    /// convert a fourcc code to a string
    static const char* FourCCToString(nFourCC, char* buf, int bufSize);

private:
    static nVariableServer* Singleton;

    class VariableDeclaration
    {
    public:
        /// default constructor
        VariableDeclaration();
        /// constructor
        VariableDeclaration(const char* name, nFourCC fourcc);
        /// constructor with name only (fourcc will be invalid)
        VariableDeclaration(const char* name);
        /// constructor with fourcc only (name will be invalid)
        VariableDeclaration(nFourCC fourcc);
        /// get name
        const char* GetName() const;
        /// get fourcc code
        nFourCC GetFourCC() const;
        /// check if variable name is valid
        bool IsNameValid() const;
        /// check if variable fourcc is valid
        bool IsFourCCValid() const;

        nString name;
        nFourCC fourcc;
    };

    nVariableContext globalVariableContext;
    nArray<VariableDeclaration> registry;
};

//------------------------------------------------------------------------------
/*
*/
inline
nVariableServer*
nVariableServer::Instance()
{
    n_assert(Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
/*
*/
inline
nVariableServer::VariableDeclaration::VariableDeclaration() :
    fourcc(0)
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
inline
nVariableServer::VariableDeclaration::VariableDeclaration(const char* n) :
    name(n),
    fourcc(0)
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
inline
nVariableServer::VariableDeclaration::VariableDeclaration(nFourCC fcc) :
    fourcc(fcc)
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
inline
nVariableServer::VariableDeclaration::VariableDeclaration(const char* n, nFourCC fcc) :
    name(n),
    fourcc(fcc)
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
inline
const char*
nVariableServer::VariableDeclaration::GetName() const
{
    return this->name.IsEmpty() ? 0 : this->name.Get();
}

//------------------------------------------------------------------------------
/*
*/
inline
nFourCC
nVariableServer::VariableDeclaration::GetFourCC() const
{
    return this->fourcc;
}

//------------------------------------------------------------------------------
/*
*/
inline
bool
nVariableServer::VariableDeclaration::IsNameValid() const
{
    return !(this->name.IsEmpty());
}

//------------------------------------------------------------------------------
/*
*/
inline
bool
nVariableServer::VariableDeclaration::IsFourCCValid() const
{
    return (0 != this->fourcc);
}

//------------------------------------------------------------------------------
/*
    Converts a string to a fourcc code.
*/
inline
nFourCC
nVariableServer::StringToFourCC(const char* str)
{
    n_assert(str);

    // create a valid FourCC even if the string is not,
    // fill invalid stuff with spaces
    char c[4] = {' ',' ',' ',' '};
    int i;
    for (i = 0; i < 4; i++)
    {
        if (0 == str[i])
        {
            break;
        }
        else if (isalnum(str[i]))
        {
            c[i] = str[i];
        }
    }
    return MAKE_FOURCC(c[0], c[1], c[2], c[3]);
}

//------------------------------------------------------------------------------
/*
    Convertes a fourcc code to a string.
*/
inline
const char*
nVariableServer::FourCCToString(nFourCC fourcc, char* buf, int bufSize)
{
    n_assert(bufSize >= 5);
    buf[0] = (fourcc)     & 0xff;
    buf[1] = (fourcc>>8)  & 0xff;
    buf[2] = (fourcc>>16) & 0xff;
    buf[3] = (fourcc>>24) & 0xff;
    buf[4] = 0;
    return buf;
}

//------------------------------------------------------------------------------
/**
    Get reference to global variable context.
*/
inline
const nVariableContext&
nVariableServer::GetGlobalVariableContext() const
{
    return this->globalVariableContext;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nVariableServer::GlobalVariableExists(nVariable::Handle varHandle) const
{
    return (0 != this->globalVariableContext.GetVariable(varHandle));
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nVariableServer::GlobalVariableExists(const char* varName) const
{
    return this->GlobalVariableExists(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
void
nVariableServer::SetGlobalVariable(const nVariable& var)
{
    this->globalVariableContext.SetVariable(var);
}

//------------------------------------------------------------------------------
/*
*/
inline
const nVariable*
nVariableServer::GetGlobalVariable(nVariable::Handle varHandle) const
{
    return this->globalVariableContext.GetVariable(varHandle);
}

//------------------------------------------------------------------------------
/*
*/
inline
const nVariable*
nVariableServer::GetGlobalVariable(const char* varName) const
{
    return this->GetGlobalVariable(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
float
nVariableServer::GetFloatVariable(const char* varName) const
{
    return this->GetFloatVariable(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
const vector4&
nVariableServer::GetVectorVariable(const char* varName) const
{
    return this->GetVectorVariable(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
int
nVariableServer::GetIntVariable(const char* varName) const
{
    return this->GetIntVariable(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
const char*
nVariableServer::GetStringVariable(const char* varName) const
{
    return this->GetStringVariable(this->FindVariableHandleByName(varName));
}

//------------------------------------------------------------------------------
/*
*/
inline
void
nVariableServer::SetFloatVariable(const char* varName, float v)
{
    this->SetFloatVariable(this->GetVariableHandleByName(varName), v);
}

//------------------------------------------------------------------------------
/*
*/
inline
void
nVariableServer::SetVectorVariable(const char* varName, const vector4& v)
{
    this->SetVectorVariable(this->GetVariableHandleByName(varName), v);
}

//------------------------------------------------------------------------------
/*
*/
inline
void
nVariableServer::SetIntVariable(const char* varName, int i)
{
    this->SetIntVariable(this->GetVariableHandleByName(varName), i);
}

//------------------------------------------------------------------------------
/*
*/
inline
void
nVariableServer::SetStringVariable(const char* varName, const char* s)
{
    this->SetStringVariable(this->GetVariableHandleByName(varName), s);
}

//------------------------------------------------------------------------------
#endif
