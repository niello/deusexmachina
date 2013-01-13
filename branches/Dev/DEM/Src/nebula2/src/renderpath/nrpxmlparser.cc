//------------------------------------------------------------------------------
//  nrpxmlparser.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpxmlparser.h"
#include <Render/PassGeometry.h>
#include <Render/PassOcclusion.h>
#include <Render/PassPosteffect.h>
#include <Data/DataServer.h>
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrpphase.h"
#include "renderpath/nrpsequence.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nRpXmlParser::nRpXmlParser() :
    xmlDocument(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpXmlParser::~nRpXmlParser()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
nRpXmlParser::OpenXml(const nString& FileName)
{
    n_assert(FrameShader.isvalid());
    n_assert(0 == this->xmlDocument);

	nString mangledPath = DataSrv->ManglePath(FileName);
    this->xmlDocument = n_new(tinyxml2::XMLDocument);
    if (xmlDocument->LoadFile(mangledPath.Get()) == tinyxml2::XML_SUCCESS)
    {
        tinyxml2::XMLHandle docHandle(this->xmlDocument);
        tinyxml2::XMLElement* elmRenderPath = docHandle.FirstChildElement("FrameShader").ToElement();
        n_assert(elmRenderPath);
        FrameShader->Name = CStrID(elmRenderPath->Attribute("name"));
        return true;
    }
    n_delete(this->xmlDocument);
    this->xmlDocument = 0;
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
nRpXmlParser::CloseXml()
{
    n_assert(this->xmlDocument);
    n_delete(this->xmlDocument);
    this->xmlDocument = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
nRpXmlParser::ParseXml()
{
    n_assert(FrameShader.isvalid());
    n_assert(this->xmlDocument);

    tinyxml2::XMLHandle docHandle(this->xmlDocument);
    tinyxml2::XMLElement* elmRenderPath = docHandle.FirstChildElement("FrameShader").ToElement();
    n_assert(elmRenderPath);

    // parse child elements
    tinyxml2::XMLElement* child;
    for (child = elmRenderPath->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (child->Value() == nString("Shaders")) ParseShaders(child);
        else if (child->Value() == nString("RenderTarget")) ParseRenderTarget(child);
        else if (child->Value() == nString("Float"))
        {
            this->ParseGlobalVariable(nVariable::Float, child, FrameShader);
        }
        else if (child->Value() == nString("Float4"))
        {
            this->ParseGlobalVariable(nVariable::Vector4, child, FrameShader);
        }
        else if (child->Value() == nString("Int"))
        {
            this->ParseGlobalVariable(nVariable::Int, child, FrameShader);
        }
        else if (child->Value() == nString("Texture"))
        {
            this->ParseGlobalVariable(nVariable::Object, child, FrameShader);
        }
        else if (child->Value() == nString("Pass"))
        {
            //this->ParseSection(child, FrameShader);
			this->ParsePass(child, FrameShader);
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nRpXmlParser::ParseShaders(tinyxml2::XMLElement* elm)
{
	tinyxml2::XMLElement* pShader = elm->FirstChildElement();
    do
    {
        if (HasAttr(pShader, "name") && HasAttr(pShader, "file"))
        {
			nRpShader& newShader = *FrameShader->shaders.Reserve(1);
            newShader.SetName(pShader->Attribute("name"));
            newShader.SetFilename(pShader->Attribute("file"));
			newShader.Validate();
			newShader.SetBucketIndex(FrameShader->shaders.Size() - 1);
        }
        else break;
    }
	while (pShader = pShader->NextSiblingElement());
}

//------------------------------------------------------------------------------
/**
    Parse a RenderTarget xml element.
*/
void
nRpXmlParser::ParseRenderTarget(tinyxml2::XMLElement* elm)
{
    n_assert(elm && FrameShader.isvalid());
    nRpRenderTarget& newRenderTarget = *FrameShader->renderTargets.Reserve(1);
    newRenderTarget.SetName(elm->Attribute("name"));
    newRenderTarget.SetFormat(nTexture2::StringToFormat(elm->Attribute("format")));
    newRenderTarget.SetRelSize(this->GetFloatAttr(elm, "relSize", 1.0f));
    if (this->HasAttr(elm, "width"))
    {
        newRenderTarget.SetWidth(this->GetIntAttr(elm, "width", 0));
    }
    if (this->HasAttr(elm, "height"))
    {
        newRenderTarget.SetHeight(this->GetIntAttr(elm, "height", 0));
    }
	newRenderTarget.Validate();
}

//------------------------------------------------------------------------------
/**
    Create a nVariable from XML element attributes "name" and "value".
*/
nVariable
nRpXmlParser::ParseVariable(nVariable::Type dataType, tinyxml2::XMLElement* elm)
{
    n_assert(elm);
    const char* varName = elm->Attribute("name");
    n_assert(varName);
    nVariable::Handle varHandle = nVariableServer::Instance()->GetVariableHandleByName(varName);
    nVariable newVariable(dataType, varHandle);
    switch (dataType)
    {
    case nVariable::Int:
        newVariable.SetInt(this->GetIntAttr(elm, "value", 0));
        break;

    case nVariable::Float:
        newVariable.SetFloat(this->GetFloatAttr(elm, "value", 0.0f));
        break;

    case nVariable::Vector4:
        {
            vector4 v4(0.0f, 0.0f, 0.0f, 0.0f);
            newVariable.SetVector4(this->GetVector4Attr(elm, "value", v4));
        }
        break;

    case nVariable::Object:
        {
            // initialize a texture object
            const char* filename = elm->Attribute("value");
            n_assert(filename);
            nTexture2* tex = nGfxServer2::Instance()->NewTexture(filename);
            if (!tex->IsLoaded())
            {
                tex->SetFilename(filename);
                if (!tex->Load())
                {
                    n_error("nRpXmlParser::ParseGlobalVariable(): could not load texture '%s'!", filename);
                }
            }
            newVariable.SetObj(tex);
        }
        break;

    default:
        n_error("nRpXmlParser::ParseGlobalVariable(): invalid datatype for variable '%s'!", varName);
        break;
    }
    return newVariable;
}

//------------------------------------------------------------------------------
/**
    Parse a Float, Vector4, Int or Bool element inside a RenderPath
    element (these are global variable definitions.
*/
void
nRpXmlParser::ParseGlobalVariable(nVariable::Type dataType, tinyxml2::XMLElement* elm, CFrameShader* pFrameShader)
{
    n_assert(elm && pFrameShader);
    nVariable var = this->ParseVariable(dataType, elm);
    nVariableServer::Instance()->SetGlobalVariable(var);
    pFrameShader->variableHandles.Append(var.GetHandle());
}

//------------------------------------------------------------------------------
/**
    Parse a Pass element inside a Section element.
*/
void nRpXmlParser::ParsePass(tinyxml2::XMLElement* elm, CFrameShader* pFrameShader)
{
    n_assert(elm && pFrameShader);

	const char* pPassType = elm->Attribute("type");

	PPass Pass;
	if (!pPassType) Pass = n_new(CPassGeometry);
	else if (!strcmp(pPassType, "Occlusion")) Pass = n_new(CPassOcclusion);
	else if (!strcmp(pPassType, "Posteffect")) Pass = n_new(CPassPosteffect);
	else /*if (!strcmp(pPassType, "Geometry"))*/ Pass = n_new(CPassGeometry);

	Pass->Name = CStrID(elm->Attribute("name"));
    Pass->shaderAlias = elm->Attribute("shader");

	Pass->pFrameShader = pFrameShader;

	nString renderTargetName("renderTarget");
    int i = 0;
    while (this->HasAttr(elm, renderTargetName.Get()))
    {
        Pass->renderTargetNames[i] = elm->Attribute(renderTargetName.Get());
        renderTargetName.Set("renderTarget");
        renderTargetName.AppendInt(++i);
    }

    Pass->ClearFlags = 0;
    if (this->HasAttr(elm, "clearColor"))
    {
        Pass->ClearFlags |= nGfxServer2::ColorBuffer;
        vector4 Color = this->GetVector4Attr(elm, "clearColor", vector4(0.0f, 0.0f, 0.0f, 1.0f));
		Pass->ClearColor = N_COLORVALUE(Color.x, Color.y, Color.z, Color.w);
    }
    if (this->HasAttr(elm, "clearDepth"))
    {
        Pass->ClearFlags |= nGfxServer2::DepthBuffer;
        Pass->ClearDepth = this->GetFloatAttr(elm, "clearDepth", 1.0f);
    }
    if (this->HasAttr(elm, "clearStencil"))
    {
        Pass->ClearFlags |= nGfxServer2::StencilBuffer;
        Pass->ClearStencil = this->GetIntAttr(elm, "clearStencil", 0);
    }
    if (HasAttr(elm, "technique")) Pass->technique = elm->Attribute("technique");

    // parse children
    tinyxml2::XMLElement* child;
    for (child = elm->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (child->Value() == nString("Float"))
        {
            this->ParseShaderState(nShaderState::Float, child, Pass);
        }
        else if (child->Value() == nString("Float4"))
        {
            this->ParseShaderState(nShaderState::Float4, child, Pass);
        }
        else if (child->Value() == nString("Int"))
        {
            this->ParseShaderState(nShaderState::Int, child, Pass);
        }
        else if (child->Value() == nString("Texture"))
        {
            this->ParseShaderState(nShaderState::Texture, child, Pass);
        }
        else if (child->Value() == nString("Phase"))
        {
            this->ParsePhase(child, Pass);
        }
    }
    pFrameShader->Passes.Append(Pass);
}

//------------------------------------------------------------------------------
/**
    Parse a shader state element inside a Pass XML element.
*/
void
nRpXmlParser::ParseShaderState(nShaderState::Type type, tinyxml2::XMLElement* elm, CPass* pPass)
{
    n_assert(elm && pPass);

    nShaderState::Param p = nShaderState::StringToParam(elm->Attribute("name"));
    nShaderArg arg(type);
    if (this->HasAttr(elm, "value"))
    {
        // this is a constant shader parameter
        switch (type)
        {
        case nShaderState::Int:
            arg.SetInt(this->GetIntAttr(elm, "value", 0));
            break;

        case nShaderState::Float:
            arg.SetFloat(this->GetFloatAttr(elm, "value", 0.0f));
            break;

        case nShaderState::Float4:
            {
                nFloat4 f4 = { 0.0f, 0.0f, 0.0f, 0.0f };
                arg.SetFloat4(this->GetFloat4Attr(elm, "value", f4));
            }
            break;

        case nShaderState::Texture:
            {
                // initialize a texture object
                const char* filename = elm->Attribute("value");
                n_assert(filename);
                nTexture2* tex = nGfxServer2::Instance()->NewTexture(filename);
                if (!tex->IsLoaded())
                {
                    tex->SetFilename(filename);
                    if (!tex->Load())
                    {
                        n_error("nRpXmlParser::ParseGlobalVariable(): could not load texture '%s'!", filename);
                    }
                }
                arg.SetTexture(tex);
            }
            break;

        default:
            n_error("nRpXmlParser::ParseShaderState(): invalid datatype '%s'!", elm->Attribute("name"));
            break;
        }
        pPass->shaderParams.SetArg(p, arg);
    }
    else if (this->HasAttr(elm, "variable"))
    {
        const char* varName = elm->Attribute("variable");
		pPass->shaderParams.SetArg(p, arg);
		nVariable::Handle h = nVariableServer::Instance()->GetVariableHandleByName(varName);
		nVariable var(h, int(p));
		pPass->varContext.AddVariable(var);
    }
}

//------------------------------------------------------------------------------
/**
    Parse a shader state element inside a Pass XML element.
*/
void
nRpXmlParser::ParseShaderState(nShaderState::Type type, tinyxml2::XMLElement* elm, nRpSequence* seq)
{
    n_assert(elm && seq);

    nShaderState::Param p = nShaderState::StringToParam(elm->Attribute("name"));
    nShaderArg arg(type);
    if (this->HasAttr(elm, "value"))
    {
        // this is a constant shader parameter
        switch (type)
        {
        case nShaderState::Int:
            arg.SetInt(this->GetIntAttr(elm, "value", 0));
            break;

        case nShaderState::Float:
            arg.SetFloat(this->GetFloatAttr(elm, "value", 0.0f));
            break;

        case nShaderState::Float4:
            {
                nFloat4 f4 = { 0.0f, 0.0f, 0.0f, 0.0f };
                arg.SetFloat4(this->GetFloat4Attr(elm, "value", f4));
            }
            break;

        case nShaderState::Texture:
            {
                // initialize a texture object
                const char* filename = elm->Attribute("value");
                n_assert(filename);
                nTexture2* tex = nGfxServer2::Instance()->NewTexture(filename);
                if (!tex->IsLoaded())
                {
                    tex->SetFilename(filename);
                    if (!tex->Load())
                    {
                        n_error("nRpXmlParser::ParseGlobalVariable(): could not load texture '%s'!", filename);
                    }
                }
                arg.SetTexture(tex);
            }
            break;

        default:
            n_error("nRpXmlParser::ParseShaderState(): invalid datatype '%s'!", elm->Attribute("name"));
            break;
        }
        seq->AddConstantShaderParam(p, arg);
    }
    else if (this->HasAttr(elm, "variable"))
    {
        const char* varName = elm->Attribute("variable");
        seq->AddVariableShaderParam(varName, p, arg);
    }
}

//------------------------------------------------------------------------------
/**
    Parse a Phase XML element.
*/
void
nRpXmlParser::ParsePhase(tinyxml2::XMLElement* elm, CPass* pPass)
{
    n_assert(elm && pPass);

    nRpPhase& newPhase = *((CPassGeometry*)pPass)->phases.Reserve(1);

    // read attributes
	newPhase.SetRenderPath(pPass->pFrameShader);
    newPhase.SetName(elm->Attribute("name"));
    newPhase.SetShaderAlias(elm->Attribute("shader"));
    newPhase.SetSortingOrder(nRpPhase::StringToSortingOrder(elm->Attribute("sort")));
    newPhase.SetLightMode(nRpPhase::StringToLightMode(elm->Attribute("lightMode")));
    if (this->HasAttr(elm, "technique"))
    {
        newPhase.SetTechnique(elm->Attribute("technique"));
    }

    // read Sequence elements
    tinyxml2::XMLElement* child;
    for (child = elm->FirstChildElement("Sequence"); child; child = child->NextSiblingElement("Sequence"))
    {
        this->ParseSequence(child, &newPhase);
    }
}

//------------------------------------------------------------------------------
/**
    Parse a Sequence XML element.
*/
void
nRpXmlParser::ParseSequence(tinyxml2::XMLElement* elm, nRpPhase* phase)
{
    n_assert(elm && phase);
    nRpSequence newSequence;
	newSequence.SetRenderPath(phase->GetRenderPath());
    newSequence.SetShaderAlias(elm->Attribute("shader"));
    if (this->HasAttr(elm, "technique"))
    {
        newSequence.SetTechnique(elm->Attribute("technique"));
    }
    newSequence.SetShaderUpdatesEnabled(this->GetBoolAttr(elm, "shaderUpdates", true));
    newSequence.SetFirstLightAlphaEnabled(this->GetBoolAttr(elm, "firstLightAlpha", false));
    newSequence.SetMvpOnlyHint(this->GetBoolAttr(elm, "mvpOnly", false));

    // parse children
    tinyxml2::XMLElement* child;
    for (child = elm->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (child->Value() == nString("Float"))
        {
            this->ParseShaderState(nShaderState::Float, child, &newSequence);
        }
        else if (child->Value() == nString("Float4"))
        {
            this->ParseShaderState(nShaderState::Float4, child, &newSequence);
        }
        else if (child->Value() == nString("Int"))
        {
            this->ParseShaderState(nShaderState::Int, child, &newSequence);
        }
        else if (child->Value() == nString("Texture"))
        {
            this->ParseShaderState(nShaderState::Texture, child, &newSequence);
        }
    }
    phase->AddSequence(newSequence);
}
