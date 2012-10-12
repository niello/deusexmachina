//------------------------------------------------------------------------------
//  ngfxserver2_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/ngfxserver2.h"
#include "resource/nresourceserver.h"
#include "gfx2/ntexture2.h"
#include "gfx2/nmesh2.h"
#include "gfx2/nshader2.h"
#include "gfx2/nmesharray.h"
#include <Resources/ResourceServer.h>

nGfxServer2* nGfxServer2::Singleton = 0;

//------------------------------------------------------------------------------
/**
*/
nGfxServer2::nGfxServer2() :
    displayOpen(false),
    inBeginFrame(false),
    inBeginScene(false),
    inBeginLines(false),
    inBeginShapes(false),
    vertexRangeFirst(0),
    vertexRangeNum(0),
    indexRangeFirst(0),
    indexRangeNum(0),
    featureSetOverride(InvalidFeatureSet),
    cursorVisibility(System),
    cursorDirty(true),
    inDialogBoxMode(false),
    gamma(1.0f),
    brightness(0.5f),
    contrast(0.5f),
    fontScale(1.0f),
    fontMinHeight(12),
    deviceIdentifier(GenericDevice),
    refRenderTargets(MaxRenderTargets),
    lightingType(Off),
    scissorRect(vector2(0.0f, 0.0f), vector2(1.0f, 1.0f)),
    hints(CountStats)
{
    n_assert(0 == Singleton);
    Singleton = this;

    int i;
    for (i = 0; i < NumTransformTypes; i++)
    {
        this->transformTopOfStack[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
nGfxServer2::~nGfxServer2()
{
    n_assert(0 != Singleton);
    Singleton = 0;
}

//------------------------------------------------------------------------------
/**
    Create a new static mesh object.

    @param  RsrcName    a resource name for resource sharing
    @return             a new nMesh2 object
*/
nMesh2*
nGfxServer2::NewMesh(const nString& /*RsrcName*/)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Create a new static mesh array object.

    @return             a new nMeshArray object
*/
nMeshArray*
nGfxServer2::NewMeshArray(const nString& /*RsrcName*/)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Create a new shared texture object.

    @param  RsrcName    a resource name for resource sharing
    @return             a new nTexture2 object
*/
nTexture2*
nGfxServer2::NewTexture(const nString& /*RsrcName*/)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Create a new shared shader object

    @param  RsrcName    a resource name for resource sharing
    @return             a new nShader2 object
*/
nShader2*
nGfxServer2::NewShader(const nString& /*RsrcName*/)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Create a new shared instance stream object.

    @param  RsrcName    a resource name for resource sharing (0 if no sharing)
    @return             pointer to a new nInstanceStream object
*/
nInstanceStream*
nGfxServer2::NewInstanceStream(const nString& RsrcName)
{
    return (nInstanceStream*)nResourceServer::Instance()->NewResource("ninstancestream", RsrcName, nResource::Other);
}

//------------------------------------------------------------------------------
/**
    Create a new occlusion query object.

    @return     pointer to a new nOcclusionQuery object
*/
nOcclusionQuery*
nGfxServer2::NewOcclusionQuery()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Create a new render target object.

    @param  RsrcName    a resource name for resource sharing
    @param  width       width of render target
    @param  height      height of render target
    @param  format      pixel format of render target
    @param  usageFlags  a combination of nTexture2::Usage flags (RenderTargetXXX only)
*/
nTexture2*
nGfxServer2::NewRenderTarget(const nString& /*RsrcName*/,
                             int /*width*/,
                             int /*height*/,
                             nTexture2::Format /*format*/,
                             int /*usageFlags*/)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Set the preferred display mode. This must be called outside
    OpenDisplay()/CloseDisplay().
*/
void
nGfxServer2::SetDisplayMode(const CDisplayMode& /*mode*/)
{
    n_error("nGfxServer2: Pure virtual function called!");
}

//------------------------------------------------------------------------------
/**
*/
const CDisplayMode&
nGfxServer2::GetDisplayMode() const
{
    n_error("nGfxServer2: Pure virtual function called!");
    static CDisplayMode dummy;
    return dummy;
}

//------------------------------------------------------------------------------
/**
    Set the window title.

    @param title        the new window title
*/
void
nGfxServer2::SetWindowTitle(const char* /*title*/)
{
    n_error("nGfxServer2: Pure virtual function called!");
}

//------------------------------------------------------------------------------
/**
    Set the current camera. Subclasses should adjust their projection matrix
    accordingly when this method is called.

    @param  camera      a camera object with valid parameters
*/
void
nGfxServer2::SetCamera(nCamera2& camera)
{
    this->camera = camera;
    this->SetTransform(Projection, camera.GetProjection());
    this->SetTransform(ShadowProjection, camera.GetShadowProjection());
}

//------------------------------------------------------------------------------
/**
    Set the current viewport. Subclasses should adjust the device accordingly.
*/
void
nGfxServer2::SetViewport(nViewport& vp)
{
    this->viewport = vp;
}

//------------------------------------------------------------------------------
/**
    Open the display.

    @return     true if display successfully opened
*/
bool
nGfxServer2::OpenDisplay()
{
    n_assert(!this->displayOpen);
    this->displayOpen = true;

    // Adjust new display to user-defined gamma (if any)
    this->AdjustGamma();

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the display.

    23-Aug-04    kims    added calling RestoreGamma().
*/
void
nGfxServer2::CloseDisplay()
{
    n_assert(this->displayOpen);
    this->displayOpen = false;

    // restore to original.
    this->RestoreGamma();
}

//------------------------------------------------------------------------------
/**
    Reset the light array. This will happen automatically in BeginScene().
*/
void
nGfxServer2::ClearLights()
{
    this->lightArray.Reset();
}

//------------------------------------------------------------------------------
/**
    Reset the light array. This will happen automatically in BeginScene().
*/
void
nGfxServer2::ClearPointLights()
{
    int index;
    for (index = 0; index < this->lightArray.Size(); index++)
    {
        if (nLight::Point == this->lightArray[index].GetType())
        {
            this->ClearLight(index);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nGfxServer2::ClearLight(int index)
{
    n_assert(index >= 0);
    this->lightArray.Erase(index);
}

//------------------------------------------------------------------------------
/**
    Add a light to the light array. Return new number of lights.
*/
int
nGfxServer2::AddLight(const nLight& light)
{
    this->lightArray.Append(light);
    return this->lightArray.Size();
}

//------------------------------------------------------------------------------
/**
    Begin rendering the current frame. This is guaranteed to be called
    exactly once per frame.
*/
bool
nGfxServer2::BeginFrame()
{
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginScene);
    this->inBeginFrame = true;

    return true;
}

//------------------------------------------------------------------------------
/**
    Finish rendering the current frame. This is guaranteed to be called
    exactly once per frame after PresentScene() has happened.
*/
void
nGfxServer2::EndFrame()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginScene);
    this->inBeginFrame = false;
}

//------------------------------------------------------------------------------
/**
    Begin rendering to the current render target. This may get called
    several times per frame.

    @return     false on error, do not call EndScene() or Present() in this case
*/
bool
nGfxServer2::BeginScene()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginScene);
    if (this->displayOpen)
    {
        // reset the light array
        this->ClearLights();
        this->inBeginScene = true;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Finish rendering to the current render target.

    @return     false on error, do not call Present() in this case.
*/
void
nGfxServer2::EndScene()
{
    n_assert(this->inBeginFrame);
    n_assert(this->inBeginScene);
    this->inBeginScene = false;
}

//------------------------------------------------------------------------------
/**
    Present the contents of the back buffer. This must be called
    outside BeginScene()/EndScene().
*/
void
nGfxServer2::PresentScene()
{
    n_assert(!this->inBeginScene);
}

//------------------------------------------------------------------------------
/**
    Clear buffers.
*/
void
nGfxServer2::Clear(int /*bufferTypes*/, float /*red*/, float /*green*/, float /*blue*/, float /*alpha*/, float /*z*/, int /*stencil*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Set the current render target at a given index (for simultaneous render targets).
    This method must be called outside BeginScene()/EndScene(). The method will
    increment the refcount of the render target object and decrement the refcount of the
    previous render target.

    @param  index   render target index
    @param  tex      the new render target, or 0 to render to the frame buffer
*/
void
nGfxServer2::SetRenderTarget(int index, nTexture2* tex)
{
    n_assert(!this->inBeginScene);
    if (tex)
    {
        tex->AddRef();
    }
    if (this->refRenderTargets[index].isvalid())
    {
        this->refRenderTargets[index]->Release();
        this->refRenderTargets[index].invalidate();
    }
    this->refRenderTargets[index] = tex;
}

//------------------------------------------------------------------------------
/**
    Set the current mesh object for rendering.

    @param  vbMesh  mesh which delivers the vertex buffer
    @param  ibMesh  mesh which delivers the index buffer
*/
void
nGfxServer2::SetMesh(nMesh2* vbMesh, nMesh2* ibMesh)
{
    this->refVbMesh = vbMesh;
    this->refIbMesh = ibMesh;
}

//------------------------------------------------------------------------------
/**
    Set the current mesh array object for rendering.

    @param  meshArray   pointer to a nMeshArray object
*/
void
nGfxServer2::SetMeshArray(nMeshArray* meshArray)
{
    this->refMeshArray = meshArray;
}

//------------------------------------------------------------------------------
/**
    Set the current shader object for rendering.

    @param  shd     pointer to a nShader2 object
*/
void
nGfxServer2::SetShader(nShader2* shd)
{
    this->refShader = shd;
}

//------------------------------------------------------------------------------
/**
    Set the current instance stream object for rendering.

    @param  stream      pointer to nInstanceStream object
*/
void
nGfxServer2::SetInstanceStream(nInstanceStream* stream)
{
    this->refInstanceStream = stream;
}

//------------------------------------------------------------------------------
/**
    Set the current mouse cursor.

    @param  cursor      a valid mouse cursor object
*/
void
nGfxServer2::SetMouseCursor(const nMouseCursor& cursor)
{
    this->curMouseCursor = cursor;
    this->cursorDirty = true;
}

//------------------------------------------------------------------------------
/**
    Get the current mouse cursor.
*/
const nMouseCursor&
nGfxServer2::GetMouseCursor() const
{
    return this->curMouseCursor;
}

//------------------------------------------------------------------------------
/**
    Set transformation matrix.

    @param  type        transform type
    @param  matrix      the 4x4 matrix
*/
void
nGfxServer2::SetTransform(TransformType type, const matrix44& matrix)
{
    n_assert(type < NumTransformTypes);
    bool updModelView = false;
    bool updModelLight = false;
    bool updViewProjection = false;
    switch (type)
    {
        case Model:
            this->transform[Model] = matrix;
            this->transform[InvModel] = matrix;
            this->transform[InvModel].invert_simple();
            updModelView = true;
            updModelLight = true;
            break;

        case View:
            this->transform[View] = matrix;
            this->transform[InvView] = matrix;
            this->transform[InvView].invert_simple();
            updModelView = true;
            updViewProjection = true;
            break;

        case Projection:
            this->transform[Projection] = matrix;
            updViewProjection = true;
            break;

        case ShadowProjection:
            this->transform[ShadowProjection] = matrix;
            break;

        case Texture0:
        case Texture1:
        case Texture2:
        case Texture3:
            this->transform[type] = matrix;
            break;

        case Light:
            this->transform[type] = matrix;
            updModelLight = true;
            break;

        default:
            n_error("nGfxServer2::SetTransform() Trying to set read-only transform type!");
            break;
    }

    if (updModelView)
    {
        this->transform[ModelView]    = this->transform[Model] * this->transform[View];
        this->transform[InvModelView] = this->transform[InvView] * this->transform[InvModel];
    }
    if (updModelLight)
    {
        this->transform[ModelLight]     = this->transform[Model] * this->transform[Light];
        this->transform[InvModelLight]  = this->transform[Light] * this->transform[InvModel];
    }
    if (updViewProjection)
    {
        this->transform[ViewProjection] = this->transform[View] * this->transform[Projection];
    }

    // update the modelview/projection matrix
    if (updModelView || updViewProjection)
    {
        this->transform[ModelViewProjection] = this->transform[ModelView] * this->transform[Projection];
    }
}

//------------------------------------------------------------------------------
/**
    Push current transformation on stack and set new matrix.

    @param  type        transform type
    @param  matrix      the 4x4 matrix
*/
void
nGfxServer2::PushTransform(TransformType type, const matrix44& matrix)
{
    n_assert(type < NumTransformTypes);
    n_assert(this->transformTopOfStack[type] < MaxTransformStackDepth);
    this->transformStack[type][this->transformTopOfStack[type]++] = this->transform[type];
    this->SetTransform(type, matrix);
}

//------------------------------------------------------------------------------
/**
    Pop transformation from stack and make it the current transform.
*/
const matrix44&
nGfxServer2::PopTransform(TransformType type)
{
    n_assert(type < NumTransformTypes);
    this->SetTransform(type, this->transformStack[type][--this->transformTopOfStack[type]]);
    return this->transform[type];
}

//------------------------------------------------------------------------------
/**
    Draw current mesh with indexed primitives.
*/
void
nGfxServer2::DrawIndexed(PrimitiveType /*primType*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Draw current mesh with non-indexed primitives.
*/
void
nGfxServer2::Draw(PrimitiveType /*primType*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Render indexed primitives without applying shader state. You have
    to call nShader2::Begin(), nShader2::Pass() and nShader2::End()
    yourself as needed.
*/
void
nGfxServer2::DrawIndexedNS(PrimitiveType /*primType*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Render non-indexed primitives without applying shader state. You have
    to call nShader2::Begin(), nShader2::Pass() and nShader2::End()
    yourself as needed.
*/
void
nGfxServer2::DrawNS(PrimitiveType /*primType*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Add text to the text buffer (OBSOLETE)
*/
void
nGfxServer2::Text(const nString& /*text*/, const vector4& /*color*/, float /*x*/, float /*y*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Render the text in the text buffer (OBSOLETE)
*/
void
nGfxServer2::DrawTextBuffer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Draw text.

    @param  text    the text to draw
    @param  color   the text color
    @param  rect    screen space rectangle in which to draw the text
    @param  flags   combination of CFont::RenderFlags
*/
void
nGfxServer2::DrawText(const nString& /*text*/, const vector4& /*color*/, const rectangle& /*rect*/, uint /*flags*/, bool /*immediate*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Return the text extents of the given text using the current font in
    screen space coordinates.
*/
vector2
nGfxServer2::GetTextExtent(const nString& /*text*/)
{
    return vector2::zero;
}

//------------------------------------------------------------------------------
/**
    Return the supported feature set.
*/
nGfxServer2::FeatureSet
nGfxServer2::GetFeatureSet()
{
    return InvalidFeatureSet;
}

//------------------------------------------------------------------------------
/**
    Save a screenshot.
*/
bool
nGfxServer2::SaveScreenshot(const char* /*filename*/, nTexture2::FileFormat /*fileFormat*/)
{
    // implement me in subclass
    return false;
}

//------------------------------------------------------------------------------
/**
    Set cursor visibility.
*/
void
nGfxServer2::SetCursorVisibility(CursorVisibility v)
{
    this->cursorVisibility = v;
    this->cursorDirty = true;
}

//------------------------------------------------------------------------------
/**
    Get the mouse cursor visibility status.
*/
nGfxServer2::CursorVisibility
nGfxServer2::GetCursorVisibility() const
{
    return this->cursorVisibility;
}

//------------------------------------------------------------------------------
/**
    Enter dialog box mode.
*/
void
nGfxServer2::EnterDialogBoxMode()
{
    n_assert(!this->inDialogBoxMode);
    this->inDialogBoxMode = true;
}

//------------------------------------------------------------------------------
/**
    Leave dialog box mode.
*/
void
nGfxServer2::LeaveDialogBoxMode()
{
    n_assert(this->inDialogBoxMode);
    this->inDialogBoxMode = false;
}

//------------------------------------------------------------------------------
/**
    This method should return the number of currently available stencil bits
    (override in subclass).
*/
int
nGfxServer2::GetNumStencilBits() const
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    This method should return the number of currently available depth bits
    (override in subclass).
*/
int
nGfxServer2::GetNumDepthBits() const
{
    return 0;
}

//------------------------------------------------------------------------------
/**
    Begin rendering lines. Override this method in a subclass.
*/
void
nGfxServer2::BeginLines()
{
    n_assert(!this->inBeginLines);
    this->inBeginLines = true;
}

//------------------------------------------------------------------------------
/**
    Draw a 3d line strip using the current transforms.

    The following draws an unit cube with red lines.

    @code
    vector3 v[8];
    v[0].set(0.5f, -0.5f, 0.5f);
    v[1].set(0.5f, -0.5f, -0.5f);
    v[2].set(-0.5f, -0.5f, -0.5f);
    v[3].set(-0.5f, -0.5f, 0.5f);
    v[4].set(0.5f, 0.5f, 0.5f);
    v[5].set(0.5f, 0.5f, -0.5f);
    v[6].set(-0.5f, 0.5f, -0.5f);
    v[7].set(-0.5f, 0.5f, 0.5f);

    vector3 cube[16] = {
        v[1], v[0], v[4], v[5],
        v[1], v[2], v[6], v[5],
        v[3], v[2], v[6], v[7],
        v[3], v[0], v[4], v[7]
    };

    nGfxServer2::Instance()->BeginLines();
    nGfxServer2::Instance()->DrawLines3d(cube,   8, vector4(1.0f, 0.0f, 0.0f, 0.5f));
    nGfxServer2::Instance()->DrawLines3d(cube+8, 8, vector4(1.0f, 0.0f, 0.0f, 0.5f));
    nGfxServer2::Instance()->EndLines();
    @endcode
*/
void
nGfxServer2::DrawLines3d(const vector3* /*vertexList*/, int /*numVertices*/, const vector4& /*color*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Draw a 2d lines in screen space.
*/
void
nGfxServer2::DrawLines2d(const vector2* /*vertexList*/, int /*numVertices*/, const vector4& /*color*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Finish rendering lines. Override this method in a subclass.
*/
void
nGfxServer2::EndLines()
{
    n_assert(this->inBeginLines);
    this->inBeginLines = false;
}

//------------------------------------------------------------------------------
/**
    Begin rendering shapes.
*/
void
nGfxServer2::BeginShapes()
{
    n_assert(!this->inBeginShapes);
    this->inBeginShapes = true;
}

//------------------------------------------------------------------------------
/**
    Render a shape.
*/
void
nGfxServer2::DrawShape(ShapeType /*type*/, const matrix44& /*model*/, const vector4& /*color*/)
{
    n_assert(this->inBeginShapes);
}

//------------------------------------------------------------------------------
/**
    Render a shape without shader management.
*/
void
nGfxServer2::DrawShapeNS(ShapeType /*type*/, const matrix44& /*model*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Draw indexed primitives with the given model matrix with given color.
*/
void
nGfxServer2::DrawShapeIndexedPrimitives(PrimitiveType /*type*/, int /*numPrimitives*/, const vector3* /*vertexList*/, int /*numVertices*/, int /*vertexWidth*/, void* /*indices*/, IndexType /*indexType*/, const matrix44& /*model*/, const vector4& /*color*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Finish shape drawing.
*/
void
nGfxServer2::EndShapes()
{
    n_assert(this->inBeginShapes);
    this->inBeginShapes = false;
}

//------------------------------------------------------------------------------
/**
    Returns true when vertex shaders run in emulation. This is needed by
    mesh handling classes when running in the Fixed Function pipeline code
    path. The method must be overwritten in subclasses.
*/
bool
nGfxServer2::AreVertexShadersEmulated()
{
    n_error("nGfxServer2::AreVertexShadersEmulated() called!");
    return false;
}

//------------------------------------------------------------------------------
/**
    Utility function which computes a ray in world space between the eye
    and the current mouse position on the near plane.
*/
line3
nGfxServer2::ComputeWorldMouseRay(const vector2& mousePos, float length)
{
    // get the current Nebula camera description and view matrix
    nCamera2& nebCamera = this->GetCamera();
    const matrix44& viewMatrix = this->GetTransform(nGfxServer2::InvView);

    // Compute mouse position in world coordinates.
    vector3 screenCoord3D((mousePos.x - 0.5f) * 2.0f, (mousePos.y - 0.5f) * 2.0f, 1.0f);
    vector3 viewCoord = nebCamera.GetInvProjection() * screenCoord3D;
    vector3 localMousePos = viewCoord * nebCamera.GetNearPlane() * 1.1f;
    localMousePos.y = -localMousePos.y;
    vector3 worldMousePos = viewMatrix * localMousePos;
    vector3 worldMouseDir = worldMousePos - viewMatrix.pos_component();
    worldMouseDir.norm();
    worldMouseDir *= length;

    return line3(worldMousePos, worldMousePos + worldMouseDir);
}

//------------------------------------------------------------------------------
/**
    Set the current scissor rectangle in virtual screen space coordinate
    (top-left is (0.0f, 0.0f), bottom-right is (1.0f, 1.0f)). This
    method doesn't enable or disable the scissor rectangle, this must be
    done externally in the shader. The default scissor rectangle
    is ((0.0f, 0.0f), (1.0f, 1.0f)).
*/
void
nGfxServer2::SetScissorRect(const rectangle& rect)
{
    this->scissorRect = rect;
}

//------------------------------------------------------------------------------
/**
    Set user defined clip planes in clip space. Clip space is where
    outgoing vertex shader vertices live in. Up to 6 clip planes
    can be defined. Provide an empty array to clear all clip planes.
*/
void
nGfxServer2::SetClipPlanes(const nArray<plane>& planes)
{
    this->clipPlanes = planes;
}
