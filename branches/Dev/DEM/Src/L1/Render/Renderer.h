#pragma once
#ifndef __DEM_L1_RENDERER_H__
#define __DEM_L1_RENDERER_H__

#include <Resources/ResourceManager.h>
#include <Render/Materials/Material.h>
#include <Render/Materials/Texture.h>
#include <Render/Geometry/Mesh.h>
#include <Data/Data.h>

// Render device interface (currently D3D9). Renderer manages shaders, shader state, shared variables,
// render targets, model, view and projection transforms, and performs actual rendering

namespace Render
{
#define RenderSrv Render::CRenderer::Instance()

class CRenderer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CRenderer);

protected:

	bool				_IsOpen;
	IDirect3DDevice9*	pD3DDevice;
	ID3DXEffectPool*	pEffectPool;

public:

	enum
	{
		MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	};

	Resources::CResourceManager<CMesh>		MeshMgr;
	Resources::CResourceManager<CTexture>	TextureMgr;
	Resources::CResourceManager<CShader>	ShaderMgr;
	Resources::CResourceManager<CMaterial>	MaterialMgr;

	CRenderer(): _IsOpen(false), pEffectPool(NULL) { __ConstructSingleton; }
	~CRenderer() { __DestructSingleton; }

	bool				Open();
	void				Close();
	bool				IsOpen() const { return _IsOpen; }

	IDirect3DDevice9*	GetD3DDevice() const { return pD3DDevice; }
	ID3DXEffectPool*	GetD3DEffectPool() const { return pEffectPool; }
};

}

#endif
