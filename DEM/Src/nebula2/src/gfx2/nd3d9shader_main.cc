//------------------------------------------------------------------------------
//  nd3d9shader_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9shader.h"
#include "gfx2/nd3d9server.h"
#include "gfx2/nd3d9texture.h"
#include "gfx2/D3DXNebula2Include.h"
#include "gfx2/nshaderparams.h"
#include <Data/DataServer.h>
#include <Data/Streams/FileStream.h>

nNebulaClass(nD3D9Shader, "nshader2");

//------------------------------------------------------------------------------
/**
*/
nD3D9Shader::nD3D9Shader() :
    effect(0),
    inBeginPass(false),
    hasBeenValidated(false),
    didNotValidate(false),
    curTechniqueNeedsSoftwareVertexProcessing(false)
{
    memset(this->parameterHandles, 0, sizeof(this->parameterHandles));
}

//------------------------------------------------------------------------------
/**
*/
nD3D9Shader::~nD3D9Shader()
{
    if (this->IsLoaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::UnloadResource()
{
    n_assert(this->IsLoaded());
    n_assert(this->effect);

    n_assert(nD3D9Server::Instance()->pD3D9Device);

    // if this is the currently set shader, unlink from gfx server
    if (nD3D9Server::Instance()->GetShader() == this)
    {
        nD3D9Server::Instance()->SetShader(0);
    }

    // release d3dx resources
    this->effect->Release();
    this->effect = 0;

    // reset current shader params
    this->curParams.Clear();

    this->SetState(Unloaded);
}

//------------------------------------------------------------------------------
/**
    Load D3DX effects file.
*/
bool
nD3D9Shader::LoadResource()
{
    n_assert(!this->IsLoaded());
    n_assert(0 == this->effect);

    HRESULT hr;
    IDirect3DDevice9* d3d9Dev = nD3D9Server::Instance()->pD3D9Device;
    n_assert(d3d9Dev);

    // mangle path name
    nString filename = this->GetFilename();
    nString mangledPath = DataSrv->ManglePath(filename);

    //load fx file...
	Data::CFileStream File;

    // open the file
	if (!File.Open(mangledPath, Data::SAM_READ))
    {
        n_error("nD3D9Shader: could not load shader file '%s'!", mangledPath.Get());
        return false;
    }

    // get size of file
    int fileSize = File.GetSize();

    // allocate data for file and read it
    void* buffer = n_malloc(fileSize);
    n_assert(buffer);
    File.Read(buffer, fileSize);
    File.Close();

    ID3DXBuffer* errorBuffer = 0;
    #if N_D3D9_DEBUG
        DWORD compileFlags = D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
    #else
        DWORD compileFlags = D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
    #endif

    // create include file handler
    nString shaderPath(mangledPath);
    CD3DXNebula2Include includeHandler(shaderPath.ExtractDirName());

    // get global effect pool from gfx server
    ID3DXEffectPool* effectPool = nD3D9Server::Instance()->GetEffectPool();
    n_assert(effectPool);

    // get the highest supported shader profiles
    LPCSTR vsProfile = D3DXGetVertexShaderProfile(d3d9Dev);
    LPCSTR psProfile = D3DXGetPixelShaderProfile(d3d9Dev);

    if (0 == vsProfile)
    {
        n_printf("Invalid Vertex Shader profile! Fallback to vs_2_0!\n");
        vsProfile = "vs_2_0";
    }

    if (0 == psProfile)
    {
        n_printf("Invalid Pixel Shader profile! Fallback to ps_2_0!\n");
        psProfile = "ps_2_0";
    }

    n_printf("Shader profiles: %s %s\n", vsProfile, psProfile);

    // create macro definitions for shader compiler
    D3DXMACRO defines[] = {
        { "VS_PROFILE", vsProfile },
        { "PS_PROFILE", psProfile },
        { 0, 0 },
    };

    // create effect
    if (compileFlags)
    {
        hr = D3DXCreateEffectFromFile(
            d3d9Dev,            // pDevice
            shaderPath.Get(),   // File name
            defines,            // pDefines
            &includeHandler,    // pInclude
            compileFlags,       // Flags
            effectPool,         // pPool
            &(this->effect),    // ppEffect
            &errorBuffer);      // ppCompilationErrors
    }
    else
    {
        hr = D3DXCreateEffect(
            d3d9Dev,            // pDevice
            buffer,             // pFileData
            fileSize,           // DataSize
            defines,            // pDefines
            &includeHandler,    // pInclude
            compileFlags,       // Flags
            effectPool,         // pPool
            &(this->effect),    // ppEffect
            &errorBuffer);      // ppCompilationErrors
    }
    n_free(buffer);

    if (FAILED(hr))
    {
        n_error("nD3D9Shader: failed to load fx file '%s' with:\n\n%s\n",
                mangledPath.Get(),
                errorBuffer ? errorBuffer->GetBufferPointer() : "No D3DX error message.");
        if (errorBuffer)
        {
            errorBuffer->Release();
        }
        return false;
    }
    n_assert(this->effect);

    // success
    this->hasBeenValidated = false;
    this->didNotValidate = false;
    this->SetState(Valid);

    // validate the effect
    this->ValidateEffect();

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetBool(nShaderState::Param p, bool val)
{
    n_assert(this->effect && (p < nShaderState::NumParameters));
    this->curParams.SetArg(p, nShaderArg(val));
    HRESULT hr = this->effect->SetBool(this->parameterHandles[p], val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetBool() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetBoolArray(nShaderState::Param p, const bool* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);

    // FIXME Floh: is the C++ bool datatype really identical to the Win32 BOOL datatype?
    HRESULT hr = this->effect->SetBoolArray(this->parameterHandles[p], (const BOOL*)array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetBoolArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetInt(nShaderState::Param p, int val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    this->curParams.SetArg(p, nShaderArg(val));
    HRESULT hr = this->effect->SetInt(this->parameterHandles[p], val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetInt() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetIntArray(nShaderState::Param p, const int* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetIntArray(this->parameterHandles[p], array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetIntArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetFloat(nShaderState::Param p, float val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    this->curParams.SetArg(p, nShaderArg(val));
    HRESULT hr = this->effect->SetFloat(this->parameterHandles[p], val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetFloat() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetFloatArray(nShaderState::Param p, const float* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetFloatArray(this->parameterHandles[p], array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetFloatArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetVector4(nShaderState::Param p, const vector4& val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    this->curParams.SetArg(p, nShaderArg(*(nFloat4*)&val));
    HRESULT hr = this->effect->SetVector(this->parameterHandles[p], (CONST D3DXVECTOR4*)&val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetVector() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetVector3(nShaderState::Param p, const vector3& val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    static vector4 v;
    v.set(val.x, val.y, val.z, 1.0f);
    this->curParams.SetArg(p, nShaderArg(*(nFloat4*)&v));
    HRESULT hr = this->effect->SetVector(this->parameterHandles[p], (CONST D3DXVECTOR4*)&v);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetVector() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetFloat4(nShaderState::Param p, const nFloat4& val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    this->curParams.SetArg(p, nShaderArg(val));
    HRESULT hr = this->effect->SetVector(this->parameterHandles[p], (CONST D3DXVECTOR4*)&val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetVector() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetFloat4Array(nShaderState::Param p, const nFloat4* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetVectorArray(this->parameterHandles[p], (CONST D3DXVECTOR4*)array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetVectorArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetVector4Array(nShaderState::Param p, const vector4* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetVectorArray(this->parameterHandles[p], (CONST D3DXVECTOR4*)array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetVectorArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetMatrix(nShaderState::Param p, const matrix44& val)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    this->curParams.SetArg(p, nShaderArg(&val));
    HRESULT hr = this->effect->SetMatrix(this->parameterHandles[p], (CONST D3DXMATRIX*)&val);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetMatrix() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetMatrixArray(nShaderState::Param p, const matrix44* array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetMatrixArray(this->parameterHandles[p], (CONST D3DXMATRIX*)array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetMatrixArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetMatrixPointerArray(nShaderState::Param p, const matrix44** array, int count)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    HRESULT hr = this->effect->SetMatrixPointerArray(this->parameterHandles[p], (CONST D3DXMATRIX**)array, count);
    #ifdef __NEBULA_STATS__
    nD3D9Server::Instance()->statsNumRenderStateChanges++;
    #endif
    n_dxtrace(hr, "SetMatrixPointerArray() on shader failed!");
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::SetTexture(nShaderState::Param p, nTexture2* tex)
{
    n_assert(this->effect && p < nShaderState::NumParameters);
    if (0 == tex)
    {
        HRESULT hr = this->effect->SetTexture(this->parameterHandles[p], 0);
        this->curParams.SetArg(p, nShaderArg((nTexture2*)0));
        #ifdef __NEBULA_STATS__
        nD3D9Server::Instance()->statsNumTextureChanges++;
        #endif
        n_dxtrace(hr, "SetTexture(0) on shader failed!");
    }
    else
    {
        uint curTexUniqueId = 0;
        if (this->curParams.IsParameterValid(p))
        {
            nTexture2* curTex = this->curParams.GetArg(p).GetTexture();
            if (curTex)
            {
                curTexUniqueId = curTex->GetUniqueId();
            }
        }

        if ((!this->curParams.IsParameterValid(p)) || (curTexUniqueId != tex->GetUniqueId()))
        {
            this->curParams.SetArg(p, nShaderArg(tex));
            HRESULT hr = this->effect->SetTexture(this->parameterHandles[p], ((nD3D9Texture*)tex)->GetBaseTexture());
            #ifdef __NEBULA_STATS__
            nD3D9Server::Instance()->statsNumTextureChanges++;
            #endif
            n_dxtrace(hr, "SetTexture() on shader failed!");
        }
    }
}

//------------------------------------------------------------------------------
/**
    Set a whole shader parameter block at once. This is slightly faster
    (and more convenient) then setting single parameters.
*/
void
nD3D9Shader::SetParams(const nShaderParams& params)
{
    int i;
    HRESULT hr;

    int numValidParams = params.GetNumValidParams();
    for (i = 0; i < numValidParams; i++)
    {
        nShaderState::Param curParam = params.GetParamByIndex(i);

        // parameter used in shader?
        D3DXHANDLE handle = this->parameterHandles[curParam];
        if (handle != 0)
        {
            const nShaderArg& curArg = params.GetArgByIndex(i);

            // early out if parameter is void
            if (curArg.GetType() == nShaderState::Void)
            {
                continue;
            }

            // avoid redundant state switches
            if ((!this->curParams.IsParameterValid(curParam)) ||
                (!(curArg == this->curParams.GetArg(curParam))))
            {
                this->curParams.SetArg(curParam, curArg);
                switch (curArg.GetType())
                {
                    case nShaderState::Bool:
                        hr = this->effect->SetBool(handle, curArg.GetBool());
                        break;

                    case nShaderState::Int:
                        hr = this->effect->SetInt(handle, curArg.GetInt());
                        break;

                    case nShaderState::Float:
                        hr = this->effect->SetFloat(handle, curArg.GetFloat());
                        break;

                    case nShaderState::Float4:
                        hr = this->effect->SetVector(handle, (CONST D3DXVECTOR4*)&(curArg.GetFloat4()));
                        break;

                    case nShaderState::Matrix44:
                        hr = this->effect->SetMatrix(handle, (CONST D3DXMATRIX*)curArg.GetMatrix44());
                        break;

                    case nShaderState::Texture:
                        hr = this->effect->SetTexture(handle, ((nD3D9Texture*)curArg.GetTexture())->GetBaseTexture());
                        break;
                }
                n_dxtrace(hr, "Failed to set shader parameter in nD3D9Shader::SetParams");
                #ifdef __NEBULA_STATS__
                nD3D9Server::Instance()->statsNumRenderStateChanges++;
                #endif
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Update the parameter handles table which maps nShader2 parameters to
    D3DXEffect parameter handles.

    - 19-Feb-04 floh    Now also recognized parameters which are not used
                        by the shader's current technique.
*/
void
nD3D9Shader::UpdateParameterHandles()
{
    n_assert(this->effect);
    HRESULT hr;

    memset(this->parameterHandles, 0, sizeof(this->parameterHandles));

    // for each parameter in the effect...
    D3DXEFFECT_DESC effectDesc = { 0 };
    hr = this->effect->GetDesc(&effectDesc);
    n_dxtrace(hr, "GetDesc() failed in UpdateParameterHandles()");
    uint curParamIndex;
    for (curParamIndex = 0; curParamIndex < effectDesc.Parameters; curParamIndex++)
    {
        D3DXHANDLE curParamHandle = this->effect->GetParameter(NULL, curParamIndex);
        n_assert(NULL != curParamHandle);

        // get the associated Nebula2 parameter index
        D3DXPARAMETER_DESC paramDesc = { 0 };
        hr = this->effect->GetParameterDesc(curParamHandle, &paramDesc);
        n_dxtrace(hr, "GetParameterDesc() failed in UpdateParameterHandles()");
        nShaderState::Param nebParam = nShaderState::StringToParam(paramDesc.Name);
        if (nebParam != nShaderState::InvalidParameter)
        {
            this->parameterHandles[nebParam] = curParamHandle;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Return true if parameter is used by effect.
*/
bool
nD3D9Shader::IsParameterUsed(nShaderState::Param p)
{
    n_assert(p < nShaderState::NumParameters);
    return (0 != this->parameterHandles[p]);
}

//------------------------------------------------------------------------------
/**
    Find the first valid technique and set it as current.
    This sets the hasBeenValidated and didNotValidate members
*/
void
nD3D9Shader::ValidateEffect()
{
    n_assert(!this->hasBeenValidated);
    n_assert(this->effect);
    n_assert(nD3D9Server::Instance()->pD3D9Device);
    IDirect3DDevice9* pD3D9Device = nD3D9Server::Instance()->pD3D9Device;
    n_assert(pD3D9Device);
    HRESULT hr;

    // get current vertex processing state
    bool origSoftwareVertexProcessing = nD3D9Server::Instance()->GetSoftwareVertexProcessing();

    // set to hardware vertex processing (this could fail if it's a pure software processing device)
    nD3D9Server::Instance()->SetSoftwareVertexProcessing(false);

    // set on first technique that validates correctly
    D3DXHANDLE technique = NULL;
    hr = this->effect->FindNextValidTechnique(0, &technique);

    // NOTE: DON'T change this to SUCCEEDED(), since FindNextValidTechnique() may
    // return S_FALSE, which the SUCCEEDED() macro interprets as a success code!
    if (D3D_OK == hr)
    {
        // technique could be validated
        D3DXTECHNIQUE_DESC desc;
        this->effect->GetTechniqueDesc(this->effect->GetTechnique(0), &desc);
        this->SetTechnique(desc.Name);
        this->hasBeenValidated = true;
        this->didNotValidate = false;
        this->UpdateParameterHandles();
    }
    else
    {
        // shader did not validate with hardware vertex processing, try with software vertex processing
        nD3D9Server::Instance()->SetSoftwareVertexProcessing(true);
        hr = this->effect->FindNextValidTechnique(0, &technique);
        this->hasBeenValidated = true;

        // NOTE: DON'T change this to SUCCEEDED(), since FindNextValidTechnique() may
        // return S_FALSE, which the SUCCEEDED() macro interprets as a success code!
        if (D3D_OK == hr)
        {
            // success with software vertex processing
            n_printf("nD3D9Shader() info: shader '%s' needs software vertex processing\n",  this->GetFilename());
            D3DXTECHNIQUE_DESC desc;
            this->effect->GetTechniqueDesc(this->effect->GetTechnique(0), &desc);
            this->SetTechnique(desc.Name);
            this->didNotValidate = false;
            this->UpdateParameterHandles();
        }
        else
        {
            // NOTE: looks like this has been fixed in the April 2005 SDK...

            // shader didn't validate at all, this may happen although the shader is valid
            // on older nVidia cards if the effect has a vertex shader, thus we simply force
            // the first technique in the file as current
            n_printf("nD3D9Shader() warning: shader '%s' did not validate!\n", this->GetFilename());

            // NOTE: this works around the dangling "BeginPass()" in D3DX when a shader did
            // not validate (reproducible on older nVidia cards)
            this->effect->EndPass();
            D3DXTECHNIQUE_DESC desc;
            this->effect->GetTechniqueDesc(this->effect->GetTechnique(0), &desc);
            this->SetTechnique(desc.Name);
            this->didNotValidate = false;
            this->UpdateParameterHandles();
        }
    }

    // restore original software processing mode
    nD3D9Server::Instance()->SetSoftwareVertexProcessing(origSoftwareVertexProcessing);
}

//------------------------------------------------------------------------------
/**
    This switches between hardware and software processing mode, as needed
    by this shader.
*/
void
nD3D9Shader::SetVertexProcessingMode()
{
    nD3D9Server* d3d9Server = (nD3D9Server*)nGfxServer2::Instance();
    d3d9Server->SetSoftwareVertexProcessing(this->curTechniqueNeedsSoftwareVertexProcessing);
}

//------------------------------------------------------------------------------
/**
    05-Jun-04   floh    saveState parameter
    26-Sep-04   floh    I misread the save state docs for DX9.0c, state saving
                        flags now correct again
*/
int
nD3D9Shader::Begin(bool saveState)
{
    n_assert(this->effect);

    // check if we already have been validated, if not, find the first
    // valid technique and set it as current
    if (!this->hasBeenValidated)
    {
        this->ValidateEffect();
    }

    if (this->didNotValidate)
    {
        return 0;
    }
    // start rendering the effect
    UINT numPasses;
    DWORD flags;
    if (saveState)
    {
        // save all state
        flags = 0;
    }
    else
    {
        // save no state
        flags = D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE | D3DXFX_DONOTSAVESHADERSTATE;
    }
    this->SetVertexProcessingMode();
    HRESULT hr = this->effect->Begin(&numPasses, flags);
    n_dxtrace(hr, "nD3D9Shader: Begin() failed on effect");
    return numPasses;
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::BeginPass(int pass)
{
    n_assert(this->effect);
    n_assert(this->hasBeenValidated && !this->didNotValidate);

    this->SetVertexProcessingMode();
#if (D3D_SDK_VERSION >= 32) //summer 2004 update sdk
    HRESULT hr = this->effect->BeginPass(pass);
    n_dxtrace(hr, "nD3D9Shader:BeginPass() failed on effect");
#else
    HRESULT hr = this->effect->Pass(pass);
    n_dxtrace(hr, "nD3D9Shader: Pass() failed on effect");
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::CommitChanges()
{
#if (D3D_SDK_VERSION >= 32) //summer 2004 update sdk
    n_assert(this->effect);
    n_assert(this->hasBeenValidated && !this->didNotValidate);

    this->SetVertexProcessingMode();
    HRESULT hr = this->effect->CommitChanges();
    n_dxtrace(hr, "nD3D9Shader: CommitChanges() failed on effect");
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::EndPass()
{
#if (D3D_SDK_VERSION >= 32) //summer 2004 update sdk
    n_assert(this->effect);
    n_assert(this->hasBeenValidated && !this->didNotValidate);

    this->SetVertexProcessingMode();
    HRESULT hr = this->effect->EndPass();
    n_dxtrace(hr, "nD3D9Shader: EndPass() failed on effect");
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9Shader::End()
{
    HRESULT hr;
    n_assert(this->effect);
    n_assert(this->hasBeenValidated);

    if (!this->didNotValidate)
    {
        this->SetVertexProcessingMode();
        hr = this->effect->End();
        n_dxtrace(hr, "End() failed on effect");
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
nD3D9Shader::HasTechnique(const char* t) const
{
    n_assert(t);
    n_assert(this->effect);
    D3DXHANDLE h = this->effect->GetTechniqueByName(t);
    return (0 != h);
}

//------------------------------------------------------------------------------
/**
*/
bool
nD3D9Shader::SetTechnique(const char* t)
{
    n_assert(t);
    n_assert(this->effect);

    // get handle to technique
    D3DXHANDLE hTechnique = this->effect->GetTechniqueByName(t);
    if (0 == hTechnique)
    {
        n_error("nD3D9Shader::SetTechnique(%s): technique not found in shader file %s!\n", t, this->GetFilename());
        return false;
    }

    // check if technique needs software vertex processing (this is the
    // case if the 3d device is a mixed vertex processing device, and
    // the current technique includes a vertex shader
    this->curTechniqueNeedsSoftwareVertexProcessing = false;

    // finally, set the technique
    HRESULT hr = this->effect->SetTechnique(hTechnique);
    if (FAILED(hr))
    {
        n_printf("nD3D9Shader::SetTechnique(%s) on shader %s failed!\n", t, this->GetFilename());
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
const char*
nD3D9Shader::GetTechnique() const
{
    n_assert(this->effect);
    return this->effect->GetCurrentTechnique();
}

//------------------------------------------------------------------------------
/**
    This converts a D3DX parameter handle to a nShaderState::Param.
*/
nShaderState::Param
nD3D9Shader::D3DXParamToShaderStateParam(D3DXHANDLE h)
{
    int i;
    for (i = 0; i < nShaderState::NumParameters; i++)
    {
        if (this->parameterHandles[i] == h)
        {
            return (nShaderState::Param)i;
        }
    }
    // fallthrough: invalid handle
    return nShaderState::InvalidParameter;
}

//------------------------------------------------------------------------------
/**
    This method is called when the d3d device is lost.
*/
void
nD3D9Shader::OnLost()
{
    n_assert(Lost != this->GetState());
    n_assert(this->effect);
    this->effect->OnLostDevice();
    this->SetState(Lost);

    // flush my current parameters (important! otherwise, seemingly redundant
    // state will not be set after OnRestore())!
    this->curParams.Clear();
}

//------------------------------------------------------------------------------
/**
    This method is called when the d3d device has been restored.
*/
void
nD3D9Shader::OnRestored()
{
    n_assert(Lost == this->GetState());
    n_assert(this->effect);
    this->effect->OnResetDevice();
    this->SetState(Valid);
}
