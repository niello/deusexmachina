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

namespace Render
{
#define RenderSrv Render::CRenderServer::Instance()

class CRenderServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CRenderServer);

protected:

	//!!!OLD!
	friend class nD3D9Server;

	enum { MaxShaderFeatureCount = sizeof(DWORD) * 8 };

	bool								_IsOpen;

	nDictionary<CStrID, PVertexLayout>	VertexLayouts;
	nDictionary<CStrID, int>			ShaderFeatures;

	CDisplay							Display;
	PRenderTarget						DefaultRT;

	UINT								D3DAdapter;
	D3DPRESENT_PARAMETERS				D3DPresentParams;
	IDirect3D9*							pD3D;
	IDirect3DDevice9*					pD3DDevice;
	ID3DXEffectPool*					pEffectPool;

	bool				CreateDevice();
	void				ReleaseDevice();
	void				SetupBufferFormats();

public:

	enum
	{
		MaxRenderTargetCount = 4,
		MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	};

	Resources::CResourceManager<CMesh>		MeshMgr;
	Resources::CResourceManager<CTexture>	TextureMgr;
	Resources::CResourceManager<CShader>	ShaderMgr;
	Resources::CResourceManager<CMaterial>	MaterialMgr;

	CRenderServer(): _IsOpen(false), pD3D(NULL), pD3DDevice(NULL), pEffectPool(NULL) { __ConstructSingleton; }
	~CRenderServer() { __DestructSingleton; }

	bool				Open();
	void				Close();
	bool				IsOpen() const { return _IsOpen; }
	void				ResetDevice(); // If display size changed or device lost

	bool				BeginFrame();
	void				EndFrame();
	void				Present();
	void				SaveScreenshot(/*image format,*/ Data::CStream& OutStream);

	void				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	void				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Offset = 0);
	void				SetVertexLayout(CVertexLayout* pVLayout);
	void				SetIndexBuffer(CIndexBuffer* pIB);
	void				SetPrimitiveGroup(CMeshGroup* pGroup);
	void				Draw();
	void				DrawInstanced(); //???revisit to SetInstanceBuffer(pVB, InstanceCount)?

	PVertexLayout		GetVertexLayout(const nArray<CVertexComponent>& Components);
	DWORD				ShaderFeatureStringToMask(const nString& FeatureString);
	CDisplay&			GetDisplay() { return Display; }
	UINT				GetD3DAdapter() const { return D3DAdapter; }
	IDirect3D9*			GetD3D() const { return pD3D; } //!!!static method in N3, creates D3D!
	IDirect3DDevice9*	GetD3DDevice() const { return pD3DDevice; }
	ID3DXEffectPool*	GetD3DEffectPool() const { return pEffectPool; }
};

}

#endif
