#ifndef N_D3D9SERVER_H
#define N_D3D9SERVER_H
//------------------------------------------------------------------------------
/**
    @class nD3D9Server
    @ingroup Gfx2

    D3D9 based gfx server.

    (C) 2002 RadonLabs GmbH
*/
#include "gfx2/ngfxserver2.h"
#include <Events/Events.h>
#include <Events/Subscription.h>

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#include <d3dx9.h>
#include <dxerr.h>

#if D3D_SDK_VERSION < 32
#error You must be using the DirectX 9 October 2004 Update SDK!  You may download it from http://www.microsoft.com/downloads/details.aspx?FamilyID=b7bc31fa-2df1-44fd-95a4-c2555446aed4&DisplayLang=en
#endif

//------------------------------------------------------------------------------
//  Debugging definitions (for shader debugging etc...)
//------------------------------------------------------------------------------
#define N_D3D9_USENVPERFHUD (0)

#define N_D3D9_DEBUG (0)

#if N_D3D9_USENVPERFHUD
#define N_D3D9_DEVICETYPE D3DDEVTYPE_REF
#define N_D3D9_ADAPTER (nD3D9Server::Instance()->pD3D9->GetAdapterCount() - 1)
#else
#define N_D3D9_DEVICETYPE D3DDEVTYPE_HAL
#define N_D3D9_ADAPTER D3DADAPTER_DEFAULT
#endif

#define N_D3D9_FORCEMIXEDVERTEXPROCESSING (0)
#define N_D3D9_FORCESOFTWAREVERTEXPROCESSING (0)

//------------------------------------------------------------------------------
class nD3D9Server : public nGfxServer2
{
public:
    /// constructor
    nD3D9Server();
    /// destructor
    virtual ~nD3D9Server();
    /// get instance pointer
    static nD3D9Server* Instance();

    /// create a shared mesh object
    virtual nMesh2* NewMesh(const nString& RsrcName);
    /// create a mesh array object
    virtual nMeshArray* NewMeshArray(const nString& RsrcName);
    /// create a shared texture object
    virtual nTexture2* NewTexture(const nString& RsrcName);
    /// create a shared shader object
    virtual nShader2* NewShader(const nString& RsrcName);
    /// create a render target object
    virtual nTexture2* NewRenderTarget(const nString& RsrcName, int width, int height, nTexture2::Format fmt, int usageFlags);
    /// create a new occlusion query object
    virtual nOcclusionQuery* NewOcclusionQuery();

    /// get display mode
	virtual const CDisplayMode& GetDisplayMode() const;
   /// set the viewport
    virtual void SetViewport(nViewport& vp);
    /// open the display
    virtual bool OpenDisplay();
    /// close the display
    virtual void CloseDisplay();
    /// get the best supported feature set
    virtual FeatureSet GetFeatureSet();
    /// get window handle
    virtual HWND GetAppHwnd() const;
    /// get parent window handle
    virtual HWND GetParentHwnd() const;
    /// returns the number of available stencil bits
    virtual int GetNumStencilBits() const;
    /// returns the number of available z bits
    virtual int GetNumDepthBits() const;
    /// set scissor rect
    virtual void SetScissorRect(const rectangle& r);
    /// set or clear user defined clip planes in clip space
    virtual void SetClipPlanes(const nArray<plane>& planes);

    /// set a new render target texture
    virtual void SetRenderTarget(int index, nTexture2* t);

    /// start rendering the current frame
    virtual bool BeginFrame();
    /// start rendering to current render target
    virtual bool BeginScene();
    /// finish rendering to current render target
    virtual void EndScene();
    /// present the contents of the back buffer
    virtual void PresentScene();
    /// end rendering the current frame
    virtual void EndFrame();
    /// clear buffers
    virtual void Clear(int bufferTypes, int ARGB, float z, int stencil);

    /// reset the light array
    virtual void ClearLights();
    /// remove a light
    virtual void ClearLight(int index);
    /// add a light to the light array (reset in BeginScene)
    virtual int AddLight(const nLight& light, const matrix44& Transform);
    /// set current mesh
    virtual void SetMesh(nMesh2* vbMesh, nMesh2* ibMesh);
    /// set current mesh array, clearing the single mesh
    virtual void SetMeshArray(nMeshArray* meshArray);
    /// set current shader
    virtual void SetShader(nShader2* shader);
    /// set transform
    virtual void SetTransform(TransformType type, const matrix44& matrix);
    /// draw the current mesh with indexed primitives
    virtual void DrawIndexed(PrimitiveType primType);
    /// draw the current mesh with non-indexed primitives
    virtual void Draw(PrimitiveType primType);
    /// render indexed primitives without applying shader state (NS == No Shader)
    virtual void DrawIndexedNS(PrimitiveType primType);
    /// render non-indexed primitives without applying shader state (NS == No Shader)
    virtual void DrawNS(PrimitiveType primType);

    /// trigger the window system message pump
    virtual void Trigger();

    /// draw text (immediately)
    virtual void DrawText(const nString& text, const vector4& color, const rectangle& rect, uint flags, bool immediate = true);
    /// get text extents
    virtual vector2 GetTextExtent(const nString& text);
    /// draw accumulated text buffer
    virtual void DrawTextBuffer();

    /// enter dialog box mode (display mode must have DialogBoxMode enabled!)
    virtual void EnterDialogBoxMode();
    /// leave dialog box mode
    virtual void LeaveDialogBoxMode();

    /// begin rendering lines
    virtual void BeginLines();
    /// draw 3d lines, using the current transforms
    virtual void DrawLines3d(const vector3* vertexList, int numVertices, const vector4& color);
    /// draw 2d lines in screen space
    virtual void DrawLines2d(const vector2* vertexList, int numVertices, const vector4& color);
    /// finish line rendering
    virtual void EndLines();

    /// begin shape rendering (for debug visualizations)
    virtual void BeginShapes();
    /// draw a shape with the given model matrix with given color
    virtual void DrawShape(ShapeType type, const matrix44& model, const vector4& color);
    /// draw a shape without shader management
    virtual void DrawShapeNS(ShapeType type, const matrix44& model);
    /// draw direct primitives
    virtual void DrawShapePrimitives(PrimitiveType type, int numPrimitives, const vector3* vertexList, int vertexWidth, const matrix44& model, const vector4& color, float Size = 1.f);
    /// draw direct indexed primitives (slow, use for debug visual visualization only!)
    virtual void DrawShapeIndexedPrimitives(PrimitiveType type, int numPrimitives, const vector3* vertexList, int numVertices, int vertexWidth, void* indices, IndexType indexType, const matrix44& model, const vector4& color);
    /// end shape rendering
    virtual void EndShapes();

    /// Access Direct3D Device directly. Might be Null!
    IDirect3DDevice9* GetD3DDevice() const;
    /// Access Direct3D object directly. Might be Null!
    IDirect3D9* GetD3D() const;
    /// adjust gamma.
    virtual void AdjustGamma();
    /// restore gamma.
    virtual void RestoreGamma();
	
	void			GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;
    /// get a pointer to the global d3dx effect pool
    ID3DXEffectPool* GetEffectPool() const;
 
private:

	/// check for lost device, and reset if possible
    bool TestResetDevice();
    /// check a buffer format combination for compatibility
    bool CheckDepthFormat(D3DFORMAT adapterFormat, D3DFORMAT backbufferFormat, D3DFORMAT depthFormat);
    /// find the best possible buffer format combination
    void FindBufferFormats(CDisplayMode::EBitDepth bpp, D3DFORMAT& dispFormat, D3DFORMAT& backFormat, D3DFORMAT& zbufFormat);
    /// create the d3d device
    bool DeviceOpen();
    /// release the d3d device
    void DeviceClose();
    /// called before device destruction, or when device lost
    void OnDeviceCleanup(bool shutdown);
    /// called after device created or restored
    void OnDeviceInit(bool startup);
    /// initialize device default state
    void InitDeviceState();
    /// update the feature set member
    void UpdateFeatureSet();
    #ifdef __NEBULA_STATS__
    /// query the d3d resource manager and fill the watched variables
    void QueryStatistics();
    #endif
    /// get d3d primitive type and num primitives for indexed drawing
    int GetD3DPrimTypeAndNumIndexed(PrimitiveType primType, D3DPRIMITIVETYPE& d3dPrimType) const;
    /// get d3d primitive type and num primitives
    int GetD3DPrimTypeAndNum(PrimitiveType primType, D3DPRIMITIVETYPE& d3dPrimType) const;
    /// update the mouse cursor image and visibility
    void UpdateCursor();
   /// update shared shader parameters per frame
    void UpdatePerFrameSharedShaderParams();
    /// update shared shader parameters per scene
    void UpdatePerSceneSharedShaderParams();
    /// return number of bits for a D3DFORMAT
    int GetD3DFormatNumBits(D3DFORMAT fmt);
    /// enable/disable software vertex processing
    void SetSoftwareVertexProcessing(bool b);
    /// get current software vertex processing state
    bool GetSoftwareVertexProcessing();
    /// initialize device identifier field
    void InitDeviceIdentifier();
    /// update the scissor rectangle in Direct3D
    void UpdateScissorRect();
    /// returns the current render target size in pixels
    vector2 GetCurrentRenderTargetSize() const;
    /// check for multi-sample AA compatibility
    D3DMULTISAMPLE_TYPE CheckMultiSampleType(D3DFORMAT backbufferFormat, D3DFORMAT depthFormat, bool windowed);
    /// draw text immediately
    void DrawTextImmediate(const nString& text, const vector4& color, const rectangle& rect, uint flags);
    /// add nEnvs describing display modes (for all adapters) to the NOH
    void CreateDisplayModeEnvVars();
   
    /// get a vertex declaration for a set of vertex component flags
    IDirect3DVertexDeclaration9* NewVertexDeclaration(const int vertexCompMask);

    friend class nD3D9Mesh;
    friend class nD3D9Texture;
    friend class nD3D9Shader;

    static nD3D9Server* Singleton;

    DWORD deviceBehaviourFlags;     ///< the behavior flags at device creation time
    D3DCAPS9 devCaps;               ///< device caps
    D3DDISPLAYMODE d3dDisplayMode;  ///< the current d3d display mode
    FeatureSet featureSet;

    class TextElement
    {
    public:
        /// default constructor
		TextElement(): flags(0) {}
        /// constructor
        TextElement(const nString& t, const vector4& c, const rectangle& r, uint flg);

        nString text;
        vector4 color;
        rectangle rect;
        uint flags;
    };
    nArray<TextElement> textElements;
    ID3DXSprite* pD3DXSprite;
	ID3DXFont* pD3DFont;
    ID3DXLine* d3dxLine;                        ///< line drawing stuff
    ID3DXMesh* shapeMeshes[NumShapeTypes];      ///< shape rendering
    nRef<nD3D9Shader> refShapeShader;           ///< the shader used for rendering shapes

    D3DPRESENT_PARAMETERS presentParams;        ///< current presentation parameters
    IDirect3DSurface9* backBufferSurface;       ///< the original back buffer surface
    IDirect3DSurface9* depthStencilSurface;     ///< the original depth stencil surface
    ID3DXEffectPool* pEffectPool;                ///< global pool for shared effect parameters
    nRef<nD3D9Shader> refSharedShader;          ///< reference shader for shared effect parameters

    nKeyArray<IDirect3DVertexDeclaration9*> vertexDeclarationCache; ///< indexed by vertexCompFlags
    int curVertexComponents;                    ///< to avoid redundant vertex declaration changes

    #ifdef __NEBULA_STATS__
    IDirect3DQuery9* queryResourceManager;      ///< for querying the d3d resource manager
    nTime timeStamp;                            ///< time stamp for FPS computation

    int statsFrameCount;
    int statsNumTextureChanges;
    int statsNumRenderStateChanges;
    int statsNumDrawCalls;
    int statsNumPrimitives;
    #endif

	// NOTE: this stuff is public because WinProcs may need to access it
    IDirect3DDevice9* pD3D9Device;               ///< pointer to device object
    IDirect3D9* pD3D9;                           ///< pointer to D3D9 object

	DECLARE_EVENT_HANDLER(OnDisplaySetCursor, OnSetCursor);
	DECLARE_EVENT_HANDLER(OnDisplayPaint, OnPaint);
	DECLARE_EVENT_HANDLER(OnDisplayToggleFullscreen, OnToggleFullscreenWindowed);
	DECLARE_EVENT_HANDLER(DisplayInput, OnDisplayInput);
};

//------------------------------------------------------------------------------
/**
*/
inline
nD3D9Server*
nD3D9Server::Instance()
{
    n_assert(0 != Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
nD3D9Server::TextElement::TextElement(const nString& t, const vector4& c, const rectangle& r, uint flg) :
    text(t),
    color(c),
    rect(r),
    flags(flg)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nD3D9Server::GetD3DPrimTypeAndNumIndexed(PrimitiveType primType, D3DPRIMITIVETYPE& d3dPrimType) const
{
    int d3dNumPrimitives = 0;
    switch (primType)
    {
        case PointList:
            d3dPrimType = D3DPT_POINTLIST;
            d3dNumPrimitives = this->indexRangeNum;
            break;

        case LineList:
            d3dPrimType = D3DPT_LINELIST;
            d3dNumPrimitives = this->indexRangeNum / 2;
            break;

        case LineStrip:
            d3dPrimType = D3DPT_LINESTRIP;
            d3dNumPrimitives = this->indexRangeNum - 1;
            break;

        case TriangleList:
            d3dPrimType = D3DPT_TRIANGLELIST;
            d3dNumPrimitives = this->indexRangeNum / 3;
            break;

        case TriangleStrip:
            d3dPrimType = D3DPT_TRIANGLESTRIP;
            d3dNumPrimitives = this->indexRangeNum - 2;
            break;

        case TriangleFan:
            d3dPrimType = D3DPT_TRIANGLEFAN;
            d3dNumPrimitives = this->indexRangeNum - 2;
            break;
    }
    return d3dNumPrimitives;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nD3D9Server::GetD3DPrimTypeAndNum(PrimitiveType primType, D3DPRIMITIVETYPE& d3dPrimType) const
{
    int d3dNumPrimitives = 0;
    switch (primType)
    {
        case PointList:
            d3dPrimType = D3DPT_POINTLIST;
            d3dNumPrimitives = this->vertexRangeNum;
            break;

        case LineList:
            d3dPrimType = D3DPT_LINELIST;
            d3dNumPrimitives = this->vertexRangeNum / 2;
            break;

        case LineStrip:
            d3dPrimType = D3DPT_LINESTRIP;
            d3dNumPrimitives = this->vertexRangeNum - 1;
            break;

        case TriangleList:
            d3dPrimType = D3DPT_TRIANGLELIST;
            d3dNumPrimitives = this->vertexRangeNum / 3;
            break;

        case TriangleStrip:
            d3dPrimType = D3DPT_TRIANGLESTRIP;
            d3dNumPrimitives = this->vertexRangeNum - 2;
            break;

        case TriangleFan:
            d3dPrimType = D3DPT_TRIANGLEFAN;
            d3dNumPrimitives = this->vertexRangeNum - 2;
            break;
    }
    return d3dNumPrimitives;
}

//------------------------------------------------------------------------------
/**
*/
inline
ID3DXEffectPool*
nD3D9Server::GetEffectPool() const
{
    return this->pEffectPool;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DDevice9*
nD3D9Server::GetD3DDevice() const
{
    return this->pD3D9Device;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3D9*
nD3D9Server::GetD3D() const
{
    return this->pD3D9;
}

//------------------------------------------------------------------------------
#endif
