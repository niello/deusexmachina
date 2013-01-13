//------------------------------------------------------------------------------
//  nd3d9server_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"
#include "gfx2/nd3d9mesh.h"
#include "gfx2/nd3d9texture.h"
#include "gfx2/nd3d9shader.h"

//------------------------------------------------------------------------------
/**
    Update the device viewport.
*/
void
nD3D9Server::SetViewport(nViewport& vp)
{
    nGfxServer2::SetViewport(vp);
    if (this->pD3D9Device)
    {
        static D3DVIEWPORT9 dvp;
        dvp.X = (DWORD)vp.x;
        dvp.Y = (DWORD)vp.y;
        dvp.Width = (DWORD)vp.width;
        dvp.Height = (DWORD)vp.height;
        dvp.MinZ = vp.nearz;
        dvp.MaxZ = vp.farz;
        this->pD3D9Device->SetViewport(&dvp);
    }
}

//------------------------------------------------------------------------------
/**
    Reset lighting.
*/
void
nD3D9Server::ClearLights()
{
    n_assert(this->pD3D9Device);
    nGfxServer2::ClearLights();
}

//------------------------------------------------------------------------------
/**
    Reset lighting.
*/
void
nD3D9Server::ClearLight(int index)
{
    nGfxServer2::ClearLight(index);

    uint maxLights = this->devCaps.MaxActiveLights;
    if (index < (int)maxLights)
    {
        HRESULT hr = this->pD3D9Device->LightEnable(index, FALSE);
        n_assert(SUCCEEDED(hr));
    }
}

//------------------------------------------------------------------------------
/**
    Add a light to the light array. This will update the shared light
    effect state.
*/
int nD3D9Server::AddLight(const nLight& light, const matrix44& Transform)
{
    n_assert(light.GetRange() > 0.0f);
    int numLights = nGfxServer2::AddLight(light, Transform);

	// even if no lighting, set light transform
	this->SetTransform(nGfxServer2::Light, Transform);

    if (Shader == this->lightingType)
    {
        nShader2* shd = this->refSharedShader.get_unsafe();

        // for directional lights, the light pos shader attributes
        // actually hold the light direction
        if (shd->IsParameterUsed(nShaderState::LightPos))
			shd->SetVector3(nShaderState::LightPos,
				light.GetType() == nLight::Directional ? transform[Light].z_component() : transform[Light].pos_component());

		if (shd->IsParameterUsed(nShaderState::LightType))
            shd->SetInt(nShaderState::LightType, light.GetType());
        if (shd->IsParameterUsed(nShaderState::LightRange))
            shd->SetFloat(nShaderState::LightRange, light.GetRange());
        if (shd->IsParameterUsed(nShaderState::LightDiffuse))
            shd->SetVector4(nShaderState::LightDiffuse, light.GetDiffuse());
        if (shd->IsParameterUsed(nShaderState::LightSpecular))
            shd->SetVector4(nShaderState::LightSpecular, light.GetSpecular());
        if (shd->IsParameterUsed(nShaderState::LightAmbient))
            shd->SetVector4(nShaderState::LightAmbient, light.GetAmbient());
        if (shd->IsParameterUsed(nShaderState::ShadowIndex))
            shd->SetVector4(nShaderState::ShadowIndex, light.GetShadowLightMask());
    }
    return numLights;
}

//------------------------------------------------------------------------------
/**
    Set a transformation matrix. This will update the shared state in
    the effect pool.
*/
void
nD3D9Server::SetTransform(TransformType type, const matrix44& matrix)
{
    // let parent update the transform matrices
    nGfxServer2::SetTransform(type, matrix);

    // update the shared shader parameters
    if (this->refSharedShader.isvalid())
    {
        nD3D9Shader* shd = this->refSharedShader.get();
        bool mvpOnly = this->GetHint(MvpOnly);
        bool setMVP = false;
        bool setEyeDir = false;
        bool setEyePos = false;
        bool setModelEyePos = false;
        switch (type)
        {
        case Model:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::Model, this->transform[Model]);
                shd->SetMatrix(nShaderState::InvModel, this->transform[InvModel]);
                shd->SetMatrix(nShaderState::ModelView, this->transform[ModelView]);
                shd->SetMatrix(nShaderState::InvModelView, this->transform[InvModelView]);
            }
            setModelEyePos = true;
            setMVP = true;
            break;

        case View:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::View, this->transform[View]);
                shd->SetMatrix(nShaderState::InvView, this->transform[InvView]);
                shd->SetMatrix(nShaderState::ModelView, this->transform[ModelView]);
                shd->SetMatrix(nShaderState::InvModelView, this->transform[InvModelView]);
                setEyePos = true;
            }
            setModelEyePos = true;
            setEyeDir = true;
            setMVP = true;
            break;

        case Projection:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::Projection, this->transform[Projection]);
            }
            setMVP = true;
            break;

        case Texture0:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::TextureTransform0, this->transform[Texture0]);
            }
            break;

        case Texture1:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::TextureTransform1, this->transform[Texture1]);
            }
            break;

        case Texture2:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::TextureTransform2, this->transform[Texture2]);
            }
            break;

        case Texture3:
            if (!mvpOnly)
            {
                shd->SetMatrix(nShaderState::TextureTransform3, this->transform[Texture3]);
            }
            break;

        case Light:
            break;
        }
        if (setMVP)
        {
            shd->SetMatrix(nShaderState::ModelViewProjection, this->transform[ModelViewProjection]);
        }
        if (!mvpOnly && setEyePos)
        {
            shd->SetVector3(nShaderState::EyePos, this->transform[InvView].pos_component());
        }

        // model eye pos always needed in lighting formula
        if (setModelEyePos)
        {
            shd->SetVector3(nShaderState::ModelEyePos, this->transform[InvModelView].pos_component());
        }
        if (setEyeDir)
        {
            shd->SetVector3(nShaderState::EyeDir, -this->transform[View].z_component());
        }
    }
}

//------------------------------------------------------------------------------
/**
    Updates shared shader parameters for the current frame.
    This method is called once per frame from within BeginFrame().
*/
void
nD3D9Server::UpdatePerFrameSharedShaderParams()
{
    if (this->refSharedShader.isvalid())
    {
        nShader2* shd = this->refSharedShader;

        // update global time
        nTime time = TimeSrv->GetTime();
        shd->SetFloat(nShaderState::Time, float(time));
    }
}

//------------------------------------------------------------------------------
/**
    Updates shared shader parameters for the current scene.
    This method is called once per frame from within BeginScene().
*/
void
nD3D9Server::UpdatePerSceneSharedShaderParams()
{
    if (this->refSharedShader.isvalid())
    {
        nShader2* shd = this->refSharedShader;

        // display resolution (or better, main render target resolution)
        vector2 rtSize = this->GetCurrentRenderTargetSize();
        vector4 dispRes(rtSize.x, rtSize.y, 0.0f, 0.0f);
        shd->SetVector4(nShaderState::DisplayResolution, dispRes);
        vector4 halfPixelSize;
        halfPixelSize.x = (1.0f / rtSize.x) * 0.5f;
        halfPixelSize.y = (1.0f / rtSize.y) * 0.5f;
        shd->SetVector4(nShaderState::HalfPixelSize, halfPixelSize);
    }
}

//------------------------------------------------------------------------------
/**
    Begin rendering the current frame. This is guaranteed to be called
    exactly once per frame.
*/
bool
nD3D9Server::BeginFrame()
{
    if (nGfxServer2::BeginFrame())
    {
        // check if d3d device is in a valid state
        if (!this->TestResetDevice())
        {
            // device could not be restored at this time
            this->inBeginFrame = false;
            return false;
        }

        // update mouse cursor image if necessary
        this->UpdateCursor();

        // update shared shader parameters
        this->UpdatePerFrameSharedShaderParams();

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Finish rendering the current frame. This is guaranteed to be called
    exactly once per frame after PresentScene() has happened.
*/
void
nD3D9Server::EndFrame()
{
    #ifdef __NEBULA_STATS__
    // query statistics
    this->QueryStatistics();
    #endif

    nGfxServer2::EndFrame();
}

//------------------------------------------------------------------------------
/**
    Start rendering the scene. This can be called several times per frame
    (each render target requires its own BeginScene()/EndScene().
*/
bool
nD3D9Server::BeginScene()
{
    n_assert(this->displayOpen);

    HRESULT hr;
    if (nGfxServer2::BeginScene())
    {
        n_assert(this->pD3D9Device);
        this->inBeginScene = false;

        // update scene shader parameters
        this->UpdatePerSceneSharedShaderParams();

        // tell d3d that a new frame is about to start
        hr = this->pD3D9Device->BeginScene();
        if (FAILED(hr))
        {
            n_printf("nD3D9Server: BeginScene() on d3d device failed!\n");
            return false;
        }

        this->inBeginScene = true;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Finish rendering the scene and present the backbuffer.
*/
void
nD3D9Server::EndScene()
{
    n_assert(this->inBeginScene);
    n_assert(this->pD3D9Device);
    HRESULT hr = this->pD3D9Device->EndScene();
    n_dxtrace(hr, "EndScene() on D3D device failed!");
    nGfxServer2::EndScene();
}

//------------------------------------------------------------------------------
/**
    Clear buffers.

    - 01-Jun-04     floh    only clear stencil when current display mode has stencil

    @param  bufferTypes     a combination of nBufferType flags
                            (COLOR | DEPTH | STENCIL)
    @param  red             the red value to write into the color buffer
    @param  green           the green value to write into the color buffer
    @param  blue            the blue value to write into the color buffer
    @param  alpha           the alpha value to write into the color buffer
    @param  z               the z value to write into the depth buffer
    @param  stencil         the stencil value to write into the stencil buffer
*/
void nD3D9Server::Clear(int bufferTypes, int ARGB, float z, int stencil)
{
	if (bufferTypes == 0) return;

    DWORD flags = 0;
    if (bufferTypes & ColorBuffer)
    {
        flags |= D3DCLEAR_TARGET;
    }
    if (bufferTypes & DepthBuffer)
    {
        flags |= D3DCLEAR_ZBUFFER;
    }
    if ((bufferTypes & StencilBuffer) &&
        ((this->presentParams.AutoDepthStencilFormat == D3DFMT_D24S8) ||
         (this->presentParams.AutoDepthStencilFormat == D3DFMT_D24X4S4)))
    {
        flags |= D3DCLEAR_STENCIL;
    }

    // no stencil buffer
    HRESULT hr = this->pD3D9Device->Clear(0, NULL, flags, ARGB, z, stencil);
    n_dxtrace(hr, "In nD3D9Server::Clear(): Clear() on D3D device failed!");
}

//------------------------------------------------------------------------------
/**
    Present the scene.
*/
void nD3D9Server::PresentScene()
{
    n_assert(pD3D9Device && !inBeginScene);

#if __NEBULA_STATS__
    this->statsFrameCount++;
#endif

    HRESULT hr = pD3D9Device->Present(NULL, NULL, NULL, NULL);
    if (FAILED(hr)) n_printf("nD3D9Server::PresentScene(): failed to present scene!\n");
    nGfxServer2::PresentScene();
}

//------------------------------------------------------------------------------
/**
    Bind vertex buffer and index buffer to vertex stream 0.

    - 26-Sep-04     floh    moved the software vertex processing stuff to
                            SetShader()

    @param  vbMesh  mesh which delivers the vertex buffer
    @param  ibMesh  mesh which delivers the index buffer
*/
void
nD3D9Server::SetMesh(nMesh2* vbMesh, nMesh2* ibMesh)
{
    HRESULT hr;
    n_assert(this->pD3D9Device);

    if (0 != vbMesh)
    {
        if ((this->refVbMesh.get_unsafe() != vbMesh) || (this->refIbMesh.get_unsafe() != ibMesh))
        {
            IDirect3DVertexBuffer9* d3dVBuf  = ((nD3D9Mesh*)vbMesh)->GetVertexBuffer();
            IDirect3DVertexDeclaration9* d3dVDecl = ((nD3D9Mesh*)vbMesh)->GetVertexDeclaration();
            n_assert(d3dVBuf);
            n_assert(d3dVDecl);
            IDirect3DIndexBuffer9* d3dIBuf = 0;
            if (ibMesh->GetNumIndices() > 0)
            {
                d3dIBuf = ((nD3D9Mesh*)ibMesh)->GetIndexBuffer();
                n_assert(d3dIBuf);
            }
            UINT stride = vbMesh->GetVertexWidth() << 2;

            // set the vertex stream source
            hr = this->pD3D9Device->SetStreamSource(0, d3dVBuf, 0, stride);
            n_dxtrace(hr, "SetStreamSource() on D3D device failed!");

            // set the vertex declaration
            hr = this->pD3D9Device->SetVertexDeclaration(d3dVDecl);
            n_dxtrace(hr, "SetVertexDeclaration() on D3D device failed!");

            // indices are provided by the mesh associated with stream 0!
            hr = this->pD3D9Device->SetIndices(d3dIBuf);
            n_dxtrace(hr, "SetIndices() on D3D device failed!");
        }
    }
    else
    {
        // clear the vertex declaration
        // FIXME FLOH: Uncommented because this generates a D3D warning
        // hr = this->pD3D9Device->SetVertexDeclaration(0);
        // n_dxtrace(hr, "SetVertexDeclaration() on D3D device failed!");

        // clear vertex stream
        hr = this->pD3D9Device->SetStreamSource(0, NULL, 0, 0);
        n_dxtrace(hr, "SetStreamSource() on D3D device failed!");

        // clear the index buffer
        hr = this->pD3D9Device->SetIndices(NULL);
        n_dxtrace(hr, "SetIndices() on D3D device failed!");
    }
    nGfxServer2::SetMesh(vbMesh, ibMesh);
}

//------------------------------------------------------------------------------
/**
    Set the current shader object.
*/
void
nD3D9Server::SetShader(nShader2* shader)
{
    if (this->GetShader() != shader)
    {
        nGfxServer2::SetShader(shader);
    }
}

//------------------------------------------------------------------------------
/**
    Set the current render target at a given index (for simultaneous render targets).
    This method must be called outside BeginScene()/EndScene(). The method will
    increment the refcount of the render target object and decrement the refcount of the
    previous render target. Setting a render target of 0 at index 0 will restore
    the original back buffer as render target.

    @param  index   render target index
    @param  t       pointer to nTexture2 object or 0
*/
void
nD3D9Server::SetRenderTarget(int index, nTexture2* t)
{
    n_assert(!this->inBeginScene);
    n_assert(this->pD3D9Device);
    n_assert((index >= 0) && (index < MaxRenderTargets));

    nGfxServer2::SetRenderTarget(index, t);

    if ((DWORD)index >= this->devCaps.NumSimultaneousRTs)
    {
        return;
    }

    HRESULT hr;
    if (t)
    {
        nD3D9Texture* d3d9Tex = (nD3D9Texture*)t;
        IDirect3DSurface9* renderTarget = d3d9Tex->GetRenderTarget();
        IDirect3DSurface9* depthStencil = d3d9Tex->GetDepthStencil();
        if (renderTarget)
        {
            hr = this->pD3D9Device->SetRenderTarget(index, renderTarget);
            n_dxtrace(hr, "SetRenderTarget() on D3D device failed!");
        }
        if (depthStencil)
        {
            hr = this->pD3D9Device->SetDepthStencilSurface(depthStencil);
            n_dxtrace(hr, "SetDepthStencilSurface() on D3D device failed!");
        }
    }
    else
    {
        if (0 == index)
        {
            // restore original color and depth/stencil for main render target
            hr = this->pD3D9Device->SetRenderTarget(0, this->backBufferSurface);
            n_dxtrace(hr, "SetRenderTarget() on D3D device failed! (index == 0)");
            hr = this->pD3D9Device->SetDepthStencilSurface(this->depthStencilSurface);
            n_dxtrace(hr, "SetDepthStencilSurface() on D3D device failed!");
        }
        else
        {
            // delete render target pointer for multiple render targets
            hr = this->pD3D9Device->SetRenderTarget(index, 0);
            n_dxtrace(hr, "SetRenderTarget() on D3D device failed! (index != 0)");
        }
    }
    this->UpdateScissorRect();
}

//------------------------------------------------------------------------------
/**
    Draw the currently set mesh with indexed primitives, texture and shader to
    the current render target.

    FIXME: the multi-pass renderer should check if state actually needs to
    be applied again. This is not necessary if the effect only has 1 pass,
    and is the same effect with the same parameters as in the last
    invocation of Draw().
*/
void
nD3D9Server::DrawIndexed(PrimitiveType primType)
{
    n_assert(this->pD3D9Device && this->inBeginScene);
    HRESULT hr;

    nD3D9Shader* shader = (nD3D9Shader*)this->GetShader();
    n_assert(shader);

    // get primitive type and number of primitives
    D3DPRIMITIVETYPE d3dPrimType;
    int d3dNumPrimitives = this->GetD3DPrimTypeAndNumIndexed(primType, d3dPrimType);

    // render current geometry, probably in multiple passes
    int numPasses = shader->Begin(false);
    int curPass;
    for (curPass = 0; curPass < numPasses; curPass++)
    {
        shader->BeginPass(curPass);
        hr = this->pD3D9Device->DrawIndexedPrimitive(
            d3dPrimType,
            0,
            this->vertexRangeFirst,
            this->vertexRangeNum,
            this->indexRangeFirst,
            d3dNumPrimitives);
        n_dxtrace(hr, "DrawIndexedPrimitive() failed!");
        shader->EndPass();

        #ifdef __NEBULA_STATS__
        if (this->GetHint(CountStats))
        {
            this->statsNumDrawCalls++;
        }
        #endif
    }
    shader->End();

    #ifdef __NEBULA_STATS__
    if (this->GetHint(CountStats))
    {
        // update num primitives rendered
        this->statsNumPrimitives += d3dNumPrimitives;
    }
    #endif
}

//------------------------------------------------------------------------------
/**
    Draw the currently set mesh with non-indexed primitives.
*/
void
nD3D9Server::Draw(PrimitiveType primType)
{
    n_assert(this->pD3D9Device && this->inBeginScene);
    HRESULT hr;

    nD3D9Shader* shader = (nD3D9Shader*)this->GetShader();
    n_assert(shader);

    // get primitive type and number of primitives
    D3DPRIMITIVETYPE d3dPrimType;
    int d3dNumPrimitives = this->GetD3DPrimTypeAndNum(primType, d3dPrimType);

    // render current geometry, probably in multiple passes
    int numPasses = shader->Begin(false);
    int curPass;
    for (curPass = 0; curPass < numPasses; curPass++)
    {
        shader->BeginPass(curPass);
        hr = this->pD3D9Device->DrawPrimitive(d3dPrimType, this->vertexRangeFirst, d3dNumPrimitives);
        n_dxtrace(hr, "DrawPrimitive() failed!");
        shader->EndPass();

        #ifdef __NEBULA_STATS__
        if (this->GetHint(CountStats))
        {
            this->statsNumDrawCalls++;
        }
        #endif
    }
    shader->End();

    #ifdef __NEBULA_STATS__
    if (this->GetHint(CountStats))
    {
        // update num primitives rendered
        this->statsNumPrimitives += d3dNumPrimitives;
    }
    #endif
}

//------------------------------------------------------------------------------
/**
    Render the currently set mesh without applying any shader state.
    You must call nShader2::Begin(), nShader2::Pass() and nShader2::End()
    yourself as needed.
*/
void
nD3D9Server::DrawIndexedNS(PrimitiveType primType)
{
    n_assert(this->pD3D9Device && this->inBeginScene);

    // get primitive type and number of primitives
    D3DPRIMITIVETYPE d3dPrimType;
    int d3dNumPrimitives = this->GetD3DPrimTypeAndNumIndexed(primType, d3dPrimType);

    this->refShader->CommitChanges();

    // do single instance rendering
    HRESULT hr = this->pD3D9Device->DrawIndexedPrimitive(
        d3dPrimType,
        0,
        this->vertexRangeFirst,
        this->vertexRangeNum,
        this->indexRangeFirst,
        d3dNumPrimitives);

    n_dxtrace(hr, "DrawIndexedPrimitive() failed!");

    #ifdef __NEBULA_STATS__
    if (this->GetHint(CountStats))
    {
        // update statistics
        this->statsNumDrawCalls++;
        this->statsNumPrimitives += d3dNumPrimitives;
    }
    #endif
}

//------------------------------------------------------------------------------
/**
    Render the currently set mesh without applying any shader state.
    You must call nShader2::Begin(), nShader2::Pass() and nShader2::End()
    yourself as needed.
*/
void
nD3D9Server::DrawNS(PrimitiveType primType)
{
    n_assert(this->pD3D9Device && this->inBeginScene);
    HRESULT hr;

    // get primitive type and number of primitives
    D3DPRIMITIVETYPE d3dPrimType;
    int d3dNumPrimitives = this->GetD3DPrimTypeAndNum(primType, d3dPrimType);

    this->refShader->CommitChanges();
    hr = this->pD3D9Device->DrawPrimitive(d3dPrimType, this->vertexRangeFirst, d3dNumPrimitives);
    n_dxtrace(hr, "DrawPrimitive() failed!");

    #ifdef __NEBULA_STATS__
    if (this->GetHint(CountStats))
    {
        // update statistics
        this->statsNumDrawCalls++;
        this->statsNumPrimitives += d3dNumPrimitives;
    }
    #endif
}
