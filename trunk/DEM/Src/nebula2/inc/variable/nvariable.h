#ifndef N_VARIABLE_H
#define N_VARIABLE_H
//------------------------------------------------------------------------------
/**
    @class nVariable
    @ingroup NebulaVariableSystem

    A variable contains typed data and is identified by a variable handle.

    (C) 2002 RadonLabs GmbH
*/

#include "kernel/ntypes.h"
#include "mathlib/matrix.h"

//------------------------------------------------------------------------------
class nVariable
{
public:
    /// variable types
    enum Type
    {
        Void,
        Int,
        Float,
        Float4,
        String,
        Object,
        Matrix,
        HandleVal,
        Vector4
    };

    typedef nFourCC Handle;
    enum
    {
        InvalidHandle = 0xffffffff
    };

    /// default constructor
    nVariable();
    /// int constructor
    nVariable(Handle h, int val);
    /// float constructor
    nVariable(Handle h, float val);
    /// float4 constructor
    nVariable(Handle h, const nFloat4& val);
    /// string constructor
    nVariable(Handle h, const char* str);
    /// object constructor
    nVariable(Handle h, void* ptr);
    /// matrix constructor
    nVariable(Handle h, const matrix44& val);
    /// vector4 constructor
    nVariable(Handle h, const vector4& val);
    /// handle constructor
    nVariable(Handle h, Handle val);
    /// constructor
    nVariable(Type t, Handle h);
    /// copy constructor
    nVariable(const nVariable& rhs);
    /// destructor
    ~nVariable();
    /// assignment operator
    nVariable& operator=(const nVariable& rhs);
    /// clear content
    void Clear();
    /// set variable type
    void SetType(Type t);
    /// get variable type
    Type GetType() const;
    /// set variable handle
    void SetHandle(Handle h);
    /// get variable handle
    Handle GetHandle() const;
    /// set int content
    void SetInt(int val);
    /// get int content
    int GetInt() const;
    /// set float content
    void SetFloat(float val);
    /// get float content
    float GetFloat() const;
    /// set float4 content
    void SetFloat4(const nFloat4& v);
    /// get float4 content
    const nFloat4& GetFloat4() const;
    /// set string content (will be copied internally)
    void SetString(const char* str);
    /// get string content
    const char* GetString() const;
    /// set object content
    void SetObj(void* ptr);
    /// get object context
    void* GetObj() const;
    /// set matrix content
    void SetMatrix(const matrix44& val);
    /// get matrix content
    const matrix44& GetMatrix() const;
    /// set vector4 content
    void SetVector4(const vector4& val);
    /// get vector4 content
    const vector4& GetVector4() const;
    /// set handle content
    void SetHandleVal(Handle h);
    /// get handle content
    Handle GetHandleVal() const;

private:
    /// delete content
    void Delete();
    /// copy content
    void Copy(const nVariable& from);

    Handle handle;
    Type type;
    union
    {
        int intVal;
        float floatVal;
        nFloat4 float4Val;
        const char* stringVal;
        void* objectVal;
        matrix44* matrixVal;
        Handle handleVal;
    };
};

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable() :
    handle(InvalidHandle),
    type(Void),
    stringVal(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Type t, Handle h) :
    handle(h),
    type(t),
    stringVal(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::Delete()
{
    if ((String == this->type) && (this->stringVal))
    {
        n_free((void*) this->stringVal);
        this->stringVal = 0;
    }
    else if ((Matrix == this->type) && (this->matrixVal))
    {
        n_delete(this->matrixVal);
        this->matrixVal = 0;
    }
    this->handle = nVariable::InvalidHandle;
    this->type = Void;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::Copy(const nVariable& from)
{
    this->handle   = from.handle;
    this->type     = from.type;
    switch (from.type)
    {
        case Int:
            this->intVal = from.intVal;
            break;

        case Float:
            this->floatVal = from.floatVal;
            break;

        case Vector4:
        case Float4:
            this->float4Val = from.float4Val;
            break;

        case String:
            n_assert(0 == this->stringVal);
            if (from.stringVal)
            {
                this->stringVal = n_strdup(from.stringVal);
            }
            break;

        case Object:
            this->objectVal = from.objectVal;
            break;

        case Matrix:
            n_assert(0 == this->matrixVal);
            if (from.matrixVal)
            {
                this->matrixVal = n_new(matrix44);
                *(this->matrixVal) = *(from.matrixVal);
            }
            break;

        case HandleVal:
            this->handleVal = from.handleVal;
            break;

        default:
            n_assert(false);
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(const nVariable& rhs) :
    stringVal(0)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::~nVariable()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable&
nVariable::operator=(const nVariable& rhs)
{
    this->Delete();
    this->Copy(rhs);
    return (*this);
}

//------------------------------------------------------------------------------
/**
    Clear content, this resets the variable type, handle and content.
*/
inline
void
nVariable::Clear()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
    Set the variable's data type. This can only be invoked on a
    new or cleared variable object.
*/
inline
void
nVariable::SetType(Type t)
{
    n_assert(Void == this->type);
    n_assert(Void != t);
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::Type
nVariable::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
    Set the variable's handle, this can only be invoked on a new or cleared
    variable object.
*/
inline
void
nVariable::SetHandle(Handle h)
{
    n_assert(InvalidHandle != h);
    n_assert(InvalidHandle == this->handle);
    this->handle = h;
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::Handle
nVariable::GetHandle() const
{
    return this->handle;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetInt(int val)
{
    n_assert(Int == this->type);
    this->intVal = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nVariable::GetInt() const
{
    n_assert(Int == this->type);
    return this->intVal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetFloat(float val)
{
    n_assert(Float == this->type);
    this->floatVal = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nVariable::GetFloat() const
{
    n_assert(Float == this->type);
    return this->floatVal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetFloat4(const nFloat4& val)
{
    n_assert(Float4 == this->type);
    this->float4Val = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nFloat4&
nVariable::GetFloat4() const
{
    n_assert(Float4 == this->type);
    return this->float4Val;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetVector4(const vector4& val)
{
    n_assert(Vector4 == this->type);
    *(vector4*)&this->float4Val  = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nVariable::GetVector4() const
{
    return *(vector4*)&this->float4Val;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetString(const char* str)
{
    n_assert(String == this->type);
    n_assert(str);
    if (this->stringVal)
    {
        n_free((void*) this->stringVal);
        this->stringVal = 0;
    }
    this->stringVal = n_strdup(str);
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nVariable::GetString() const
{
    n_assert(String == this->type);
    return this->stringVal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetObj(void* ptr)
{
    n_assert(Object == this->type);
    this->objectVal = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline
void*
nVariable::GetObj() const
{
    n_assert(Object == this->type);
    return this->objectVal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetMatrix(const matrix44& val)
{
    n_assert(Matrix == this->type);
    if (0 == this->matrixVal)
    {
        this->matrixVal = n_new(matrix44);
    }
    *(this->matrixVal) = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nVariable::GetMatrix() const
{
    n_assert(Matrix == this->type);
    n_assert(this->matrixVal);
    return *(this->matrixVal);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nVariable::SetHandleVal(Handle val)
{
    n_assert(HandleVal == this->type);
    this->handleVal = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::Handle
nVariable::GetHandleVal() const
{
    n_assert(HandleVal == this->type);
    return this->handleVal;
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, int val) :
    handle(h),
    type(Int),
    intVal(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, float val) :
    handle(h),
    type(Float),
    floatVal(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, const nFloat4& val) :
    handle(h),
    type(Float4)
{
    this->SetFloat4(val);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, const vector4& val) :
    handle(h),
    type(Vector4)
{
    this->SetVector4(val);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, const char* str) :
    handle(h),
    type(String),
    stringVal(0)
{
    this->SetString(str);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, void* ptr) :
    handle(h),
    type(Object),
    objectVal(ptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, const matrix44& val) :
    handle(h),
    type(Matrix),
    matrixVal(0)
{
    this->SetMatrix(val);
}
//------------------------------------------------------------------------------
/**
*/
inline
nVariable::nVariable(Handle h, Handle val) :
    handle(h),
    type(HandleVal),
    handleVal(val)
{
    // empty
}

//------------------------------------------------------------------------------
#endif

