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

// Render device interface (currently D3D9). Renderer manages shaders, shader state, shared variables,
// render targets, model, view and projection transforms, and performs actual rendering

//!!!need FrameLog - logging of rendering calls for one particular frame!

namespace Render
{
#define RenderSrv Render::CRenderServer::Instance()

class CRenderServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CRenderServer);

public:

	enum
	{
		MaxTextureStageCount = 8,
		MaxRenderTargetCount = 4,
		MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	};

protected:

	//!!!OLD!
	friend class nD3D9Server;

	enum { MaxShaderFeatureCount = sizeof(DWORD) * 8 };

	bool								_IsOpen;
	bool								IsInsideFrame;
	DWORD								InstanceCount;	// If 0, non-instanced rendering is active

	nDictionary<CStrID, PVertexLayout>	VertexLayouts;
	nDictionary<CStrID, int>			ShaderFeatures;
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

	bool				CreateDevice();
	void				ReleaseDevice();
	void				SetupBufferFormats();

public:

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
	void				SaveScreenshot(EImageFormat ImageFormat, Data::CStream& OutStream);

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
	void				Draw();

	PVertexLayout		GetVertexLayout(const nArray<CVertexComponent>& Components);
	//???PVertexLayout		GetVertexLayout(const nString& Signature);
	EPixelFormat		GetPixelFormat(const nString& String); //???CStrID?
	DWORD				ShaderFeatureStringToMask(const nString& FeatureString);
	DWORD				GetFeatureFlagSkinned() const { return FFlagSkinned; }
	DWORD				GetFeatureFlagInstanced() const { return FFlagInstanced; }

	CDisplay&			GetDisplay() { return Display; }
	DWORD				GetFrameID() const { return FrameID; }
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

}

#endif
