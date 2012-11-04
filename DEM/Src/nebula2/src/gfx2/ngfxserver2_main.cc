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

nGfxServer2* nGfxServer2::Singleton = NULL;

nGfxServer2::nGfxServer2():
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
	n_assert(!Singleton);
	Singleton = this;
	for (int i = 0; i < NumTransformTypes; i++)
		transformTopOfStack[i] = 0;
}
//---------------------------------------------------------------------

// Create a new shared instance stream object.
nInstanceStream* nGfxServer2::NewInstanceStream(const nString& RsrcName)
{
	return (nInstanceStream*)nResourceServer::Instance()->NewResource("ninstancestream", RsrcName, nResource::Other);
}
//---------------------------------------------------------------------

// Set the current camera. Subclasses should adjust their projection matrix accordingly when this method is called
void nGfxServer2::SetCamera(nCamera2& NewCamera)
{
	Camera = NewCamera;
	SetTransform(Projection, Camera.GetProjection());
	SetTransform(ShadowProjection, Camera.GetShadowProjection());
}
//---------------------------------------------------------------------

// Open the display.
bool nGfxServer2::OpenDisplay()
{
	n_assert(!this->displayOpen);
	this->displayOpen = true;
	// Adjust new display to user-defined gamma (if any)
	this->AdjustGamma();
	return true;
}
//---------------------------------------------------------------------

// Close the display.
void nGfxServer2::CloseDisplay()
{
	n_assert(this->displayOpen);
	this->displayOpen = false;
	this->RestoreGamma();
}
//---------------------------------------------------------------------

// Begin rendering the current frame. This is guaranteed to be called exactly once per frame.
bool nGfxServer2::BeginFrame()
{
	n_assert(!this->inBeginFrame);
	n_assert(!this->inBeginScene);
	this->inBeginFrame = true;
	return true;
}
//---------------------------------------------------------------------

// Finish rendering the current frame. This is guaranteed to be called
// exactly once per frame after PresentScene() has happened.
void nGfxServer2::EndFrame()
{
	n_assert(inBeginFrame && !inBeginScene);
	inBeginFrame = false;
}
//---------------------------------------------------------------------

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
    Set the current render target at a given index (for simultaneous render targets).
    This method must be called outside BeginScene()/EndScene(). The method will
    increment the refcount of the render target object and decrement the refcount of the
    previous render target.

    @param  index   render target index
    @param  tex      the new render target, or 0 to render to the frame buffer
*/
void nGfxServer2::SetRenderTarget(int index, nTexture2* tex)
{
    n_assert(!this->inBeginScene);
    if (tex) tex->AddRef();
    if (this->refRenderTargets[index].isvalid())
    {
        this->refRenderTargets[index]->Release();
        this->refRenderTargets[index].invalidate();
    }
    this->refRenderTargets[index] = tex;
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
    Utility function which computes a ray in world space between the eye
    and the current mouse position on the near plane.
*/
line3 nGfxServer2::ComputeWorldMouseRay(const vector2& mousePos, float length)
{
    const matrix44& viewMatrix = this->GetTransform(nGfxServer2::InvView);

    // Compute mouse position in world coordinates.
    vector3 screenCoord3D((mousePos.x - 0.5f) * 2.0f, (mousePos.y - 0.5f) * 2.0f, 1.0f);
    vector3 viewCoord = GetCamera().GetInvProjection() * screenCoord3D;
    vector3 localMousePos = viewCoord * GetCamera().GetNearPlane() * 1.1f;
    localMousePos.y = -localMousePos.y;
    vector3 worldMousePos = viewMatrix * localMousePos;
    vector3 worldMouseDir = worldMousePos - viewMatrix.pos_component();
    worldMouseDir.norm();
    worldMouseDir *= length;

    return line3(worldMousePos, worldMousePos + worldMouseDir);
}