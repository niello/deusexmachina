//------------------------------------------------------------------------------
//  nrenderpath2.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrenderpath2.h"
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrpxmlparser.h"

//------------------------------------------------------------------------------
/**
*/
nRenderPath2::nRenderPath2() :
    isOpen(false),
    sequenceShaderIndex(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRenderPath2::~nRenderPath2()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Open the XML document. This will just load the XML document and
    initialize the shader path. The rest of the initialization happens
    inside nRenderPath2::Open(). This 2-step approach is necessary to
    prevent a shader initialization chicken/egg problem
*/
bool
nRenderPath2::OpenXml()
{
    this->xmlParser.SetRenderPath(this);
    if (!this->xmlParser.OpenXml())
    {
        return false;
    }
    n_assert(!this->name.IsEmpty());
    n_assert(!this->shaderPath.IsEmpty());
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the XML document. This method should be called after
    nRenderPath2::Open() to release the memory assigned to the XML
    document data.
*/
void
nRenderPath2::CloseXml()
{
    this->xmlParser.CloseXml();
}

//------------------------------------------------------------------------------
/**
    Open the render path. This will parse the xml file which describes
    the render path and configure the render path object from it.
*/
bool
nRenderPath2::Open()
{
    n_assert(!this->isOpen);
    this->sequenceShaderIndex = 0;
    if (!xmlParser.ParseXml())
    {
        return false;
    }
    this->Validate();
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the render path. This will delete all embedded objects.
*/
void
nRenderPath2::Close()
{
    n_assert(this->isOpen);
    this->name.Clear();
    this->sections.Clear();
    this->renderTargets.Clear();
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Validate the render path.
*/
void
nRenderPath2::Validate()
{
    int sectionIndex;
    int numSections = this->sections.Size();
    for (sectionIndex = 0; sectionIndex < numSections; sectionIndex++)
    {
        this->sections[sectionIndex].SetRenderPath(this);
        this->sections[sectionIndex].Validate();
    }
}

//------------------------------------------------------------------------------
/**
    Find a render target index by name. Return -1 if not found.
*/
int
nRenderPath2::FindRenderTargetIndex(const nString& n) const
{
    int i;
    int num = this->renderTargets.Size();
    for (i = 0; i < num; i++)
    {
        if (this->renderTargets[i].GetName() == n)
        {
            return i;
        }
    }
    // fallthrough: not found
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
void
nRenderPath2::AddRenderTarget(nRpRenderTarget& rt)
{
    rt.Validate();
    this->renderTargets.Append(rt);
}

//------------------------------------------------------------------------------
/**
    Find a shader definition index by its name. Return -1 if not found.
*/
int
nRenderPath2::FindShaderIndex(const nString& n) const
{
    int i;
    int num = this->shaders.Size();
    for (i = 0; i < num; i++)
    {
        if (this->shaders[i].GetName() == n)
        {
            return i;
        }
    }
    // fallthrough: not found
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
void
nRenderPath2::AddShader(nRpShader& s)
{
    s.Validate();
    this->shaders.Append(s);
    this->shaders.Back().SetBucketIndex(this->shaders.Size() - 1);
}

//------------------------------------------------------------------------------
/**
    Find a section index by name, return -1 if not found.
*/
int
nRenderPath2::FindSectionIndex(const nString& n) const
{
    int i;
    int num = this->sections.Size();
    for (i = 0; i < num; i++)
    {
        if (this->sections[i].GetName() == n)
        {
            return i;
        }
    }
    // fallthrough: not found
    return -1;
}

