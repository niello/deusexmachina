#ifndef N_RENDERPATH2_H
#define N_RENDERPATH2_H
//------------------------------------------------------------------------------
/**
    @class nRenderPath2
    @ingroup RenderPath

    @brief A render path is an abstract description of HOW a scene
    should be rendered.
    This includes things like what shaders to apply and in which order,
    what render targets to render to, highlevel differences between DX9
    and DX7 scene rendering.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "renderpath/nrpsection.h"
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrpshader.h"
#include "renderpath/nrpxmlparser.h"
#include "variable/nvariable.h"

//------------------------------------------------------------------------------
class nRenderPath2
{
public:
    /// constructor
    nRenderPath2();
    /// destructor
    ~nRenderPath2();
    /// set xml filename
    void SetFilename(const nString& n);
    /// get xml filename
    const nString& GetFilename() const;
    /// open the XML document
    bool OpenXml();
    /// close the XML document
    void CloseXml();
    /// open the object, the XML document must be open
    bool Open();
    /// close the object
    void Close();
    /// return true if currently open
    bool IsOpen() const;
    /// set the render path object's name
    void SetName(const nString& n);
    /// get the render path object's name
    const nString& GetName() const;
    /// set the shader root path
    void SetShaderPath(const nString& p);
    /// get the shader root path
    const nString& GetShaderPath() const;
    /// add a render target
    void AddRenderTarget(nRpRenderTarget& rt);
    /// find render target by name, returns -1 if not found
    int FindRenderTargetIndex(const nString& n) const;
    /// get array of render targets
    const nArray<nRpRenderTarget>& GetRenderTargets() const;
    /// get render target at index
    nRpRenderTarget& GetRenderTarget(int i) const;
    /// add a shader definition
    void AddShader(nRpShader& rt);
    /// find a shader definition by its name, returns -1 if not found
    int FindShaderIndex(const nString& n) const;
    /// returns shader definitions array
    const nArray<nRpShader>& GetShaders() const;
    /// returns shader definition at index
    nRpShader& GetShader(int i)  const;
    /// add a section
    void AddSection(nRpSection& s);
    /// find a section index by name
    int FindSectionIndex(const nString& n) const;
    /// get array of sections
    const nArray<nRpSection>& GetSections() const;
    /// get section by index
    nRpSection& GetSection(int i) const;
    /// add a global variable
    void AddVariable(const nVariable& var);
    /// get global variable handles
    const nArray<nVariable::Handle>& GetVariableHandles() const;
    /// update variable value
    void UpdateVariable(const nVariable& var);
    /// get current sequence shader index and increment by one
    int GetSequenceShaderAndIncrement();

private:
    /// validate render path resources
    void Validate();
    /// draw a full-screen quad
    void DrawQuad();

    bool isOpen;
    nString xmlFilename;
    nString name;
    nString shaderPath;
    nRpXmlParser xmlParser;
    nArray<nRpSection> sections;
    nArray<nRpRenderTarget> renderTargets;
    nArray<nRpShader> shaders;
    nArray<nVariable::Handle> variableHandles;
    int sequenceShaderIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::UpdateVariable(const nVariable& var)
{
    nVariableServer::Instance()->SetGlobalVariable(var);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRenderPath2::GetSequenceShaderAndIncrement()
{
    return this->sequenceShaderIndex++;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nRenderPath2::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::SetFilename(const nString& n)
{
    this->xmlFilename = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRenderPath2::GetFilename() const
{
    return this->xmlFilename;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRenderPath2::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpRenderTarget>&
nRenderPath2::GetRenderTargets() const
{
    return this->renderTargets;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::AddSection(nRpSection& s)
{
    this->sections.Append(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpSection>&
nRenderPath2::GetSections() const
{
    return this->sections;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpSection&
nRenderPath2::GetSection(int i) const
{
    return this->sections[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::AddVariable(const nVariable& var)
{
    nVariableServer::Instance()->SetGlobalVariable(var);
    this->variableHandles.Append(var.GetHandle());
}

//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nVariable::Handle>&
nRenderPath2::GetVariableHandles() const
{
    return this->variableHandles;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRenderPath2::SetShaderPath(const nString& p)
{
    this->shaderPath = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRenderPath2::GetShaderPath() const
{
    return this->shaderPath;
}


//------------------------------------------------------------------------------
/**
*/
inline
const nArray<nRpShader>&
nRenderPath2::GetShaders() const
{
    return this->shaders;
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpShader&
nRenderPath2::GetShader(int i) const
{
    return this->shaders[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
nRpRenderTarget&
nRenderPath2::GetRenderTarget(int i) const
{
    return this->renderTargets[i];
}

//------------------------------------------------------------------------------
#endif
