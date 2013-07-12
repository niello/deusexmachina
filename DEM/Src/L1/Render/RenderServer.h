#pragma once
#ifndef __DEM_L1_RENDER_SERVER_H__
#define __DEM_L1_RENDER_SERVER_H__

#include <Render/Display.h>
#include <Render/Materials/Material.h>
#include <Render/Materials/Texture.h>
#include <Render/RenderTarget.h>
#include <Render/Geometry/Mesh.h>
#include <Resources/ResourceManager.h>
#include <Data/Data.h>
#include <Data/DynamicEnum.h>
#include <Core/Singleton.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>

// Render device interface (currently D3D9). Renderer manages shaders, shader state, shared variables,
// render targets, model, view and projection transforms, and performs actual rendering

//!!!need FrameLog - logging of rendering calls for one particular frame!

namespace Render
{
#define RenderSrv Render::CRenderServer::Instance()

class CRenderServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CRenderServer);

public:

	enum
	{
		MaxTextureStageCount = 8, //???16?
		MaxRenderTargetCount = 4,
		MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	};

protected:

	bool								_IsOpen;
	bool								IsInsideFrame;
	bool								Wireframe;
	DWORD								InstanceCount;	// If 0, non-instanced rendering is active

	CDict<CStrID, PVertexLayout>	VertexLayouts;
	DWORD								FFlagSkinned;
	DWORD								FFlagInstanced;


	CDisplay							Display;
	PRenderTarget						DefaultRT;

	//???can write better?
	PShader								SharedShader;
	CShader::HVar						hLightAmbient;
	CShader::HVar						hEyePos;
	CShader::HVar						hViewProj;
	vector3								CurrCameraPos;
	matrix44							CurrViewProj; //???or store camera ref?

	DWORD								FrameID;
	PRenderTarget						CurrRT[MaxRenderTargetCount];
	PVertexBuffer						CurrVB[MaxVertexStreamCount];
	DWORD								CurrVBOffset[MaxVertexStreamCount];
	PVertexLayout						CurrVLayout;
	PIndexBuffer						CurrIB;
	CMeshGroup							CurrPrimGroup;
	EPixelFormat						CurrDepthStencilFormat;
	IDirect3DSurface9*					pCurrDSSurface;

	UINT								D3DAdapter;
	D3DCAPS9							D3DCaps;
	D3DPRESENT_PARAMETERS				D3DPresentParams;
	IDirect3D9*							pD3D;
	IDirect3DDevice9*					pD3DDevice;
	ID3DXEffectPool*					pEffectPool;

	DWORD								PrimsRendered;
	DWORD								DIPsRendered;

	bool				CreateDevice();
	void				ReleaseDevice();
	void				SetupDevice();
	void				SetupPresentParams();

	DECLARE_EVENT_HANDLER(OnDisplayPaint, OnDisplayPaint);
	DECLARE_EVENT_HANDLER(OnDisplayToggleFullscreen, OnToggleFullscreenWindowed);
	DECLARE_EVENT_HANDLER(OnDisplaySizeChanged, OnDisplaySizeChanged);

public:

	Data::CDynamicEnum32					ShaderFeatures;
	Resources::CResourceManager<CMesh>		MeshMgr;
	Resources::CResourceManager<CTexture>	TextureMgr;
	Resources::CResourceManager<CShader>	ShaderMgr;
	Resources::CResourceManager<CMaterial>	MaterialMgr;

	CRenderServer();
	~CRenderServer() { __DestructSingleton; }

	bool				Open();
	void				Close();
	bool				IsOpen() const { return _IsOpen; }
	void				ResetDevice(); // If display size changed or device lost

	bool				CheckCaps(ECaps Cap);

	bool				BeginFrame();
	void				EndFrame();
	void				Present();
	void				SaveScreenshot(EImageFormat ImageFormat, IO::CStream& OutStream);

	void				SetAmbientLight(const vector4& Color);
	void				SetCameraPosition(const vector3& Pos);
	void				SetViewProjection(const matrix44& VP);

	void				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	void				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	void				SetVertexLayout(CVertexLayout* pVLayout);
	void				SetIndexBuffer(CIndexBuffer* pIB);
	void				SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	void				SetPrimitiveGroup(const CMeshGroup& Group) { CurrPrimGroup = Group; }
	void				Clear(DWORD Flags, DWORD Color, float Depth, uchar Stencil);
	void				ClearScreen(DWORD Color);
	void				Draw();

	PVertexLayout		GetVertexLayout(const nArray<CVertexComponent>& Components);
	//???PVertexLayout		GetVertexLayout(const nString& Signature);
	EPixelFormat		GetPixelFormat(const nString& String); //???CStrID?
	int					GetFormatBits(EPixelFormat Format);
	DWORD				GetFeatureFlagSkinned() const { return FFlagSkinned; }
	DWORD				GetFeatureFlagInstanced() const { return FFlagInstanced; }

	void				SetWireframe(bool Wire);
	bool				IsWireframe() const { return Wireframe; }

	CDisplay&			GetDisplay() { return Display; }
	DWORD				GetFrameID() const { return FrameID; }
	DWORD				GetBackBufferWidth() const { return D3DPresentParams.BackBufferWidth; }
	DWORD				GetBackBufferHeight() const { return D3DPresentParams.BackBufferHeight; }
	UINT				GetD3DAdapter() const { return D3DAdapter; }
	IDirect3D9*			GetD3D() const { return pD3D; } //!!!static method in N3, creates D3D!
	IDirect3DDevice9*	GetD3DDevice() const { return pD3DDevice; }
	ID3DXEffectPool*	GetD3DEffectPool() const { return pEffectPool; }

	const vector3&		GetCameraPosition() const { return CurrCameraPos; }
	const matrix44&		GetViewProjection() const { return CurrViewProj; }
};

inline CRenderServer::CRenderServer():
	_IsOpen(false),
	IsInsideFrame(false),
	Wireframe(false),
	InstanceCount(0),
	hLightAmbient(NULL),
	hEyePos(NULL),
	hViewProj(NULL),
	FrameID(0),
	pD3D(NULL),
	pD3DDevice(NULL),
	pEffectPool(NULL),
	CurrDepthStencilFormat(PixelFormat_Invalid),
	pCurrDSSurface(NULL)
{
	__ConstructSingleton;
	memset(CurrVBOffset, 0, sizeof(CurrVBOffset));
}
//---------------------------------------------------------------------

inline void CRenderServer::SetAmbientLight(const vector4& Color)
{
	if (hLightAmbient) SharedShader->SetFloat4(hLightAmbient, Color);
}
//---------------------------------------------------------------------

inline void CRenderServer::SetCameraPosition(const vector3& Pos)
{
	CurrCameraPos = Pos;
	if (hEyePos) SharedShader->SetFloat4(hEyePos, vector4(Pos));
}
//---------------------------------------------------------------------

inline void CRenderServer::SetViewProjection(const matrix44& VP)
{
	CurrViewProj = VP;
	if (hViewProj) SharedShader->SetMatrix(hViewProj, CurrViewProj);
}
//---------------------------------------------------------------------

inline void CRenderServer::ClearScreen(DWORD Color)
{
	if (BeginFrame())
	{
		Clear(Clear_Color, Color, 1.f, 0);
		EndFrame();
		Present();
	}
}
//---------------------------------------------------------------------

}

#endif
