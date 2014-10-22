#pragma once
#ifndef __DEM_L1_RENDER_SERVER_H__
#define __DEM_L1_RENDER_SERVER_H__

#include <Render/Materials/Material.h>
#include <Render/Materials/Texture.h>
#include <Render/RenderTarget.h>
#include <Render/Geometry/Mesh.h>
#include <Resources/ResourceManager.h>
#include <Data/Data.h>
#include <Data/DynamicEnum.h>
#include <Data/Singleton.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

// Render device interface (currently D3D9). Renderer manages shaders, shader state, shared variables,
// render targets, model, view and projection transforms, and performs actual rendering

//!!!need FrameLog - logging of rendering calls for one particular frame!

namespace Render
{
typedef Ptr<class CFrameShader> PFrameShader;

#define RenderSrv Render::CRenderServer::Instance()

class CRenderServer: public Core::CObject
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

	DWORD								FFlagSkinned;
	DWORD								FFlagInstanced;

	CDict<CStrID, PFrameShader>			FrameShaders;
	CStrID								ScreenFrameShaderID;

	PRenderTarget						DefaultRT;

	//???can write better?
	PShader								SharedShader;
	CShader::HVar						hLightAmbient;
	CShader::HVar						hEyePos;
	CShader::HVar						hViewProj;
	vector3								CurrCameraPos;
	matrix44							CurrViewProj; //???or store camera ref?

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

	void				SetAmbientLight(const vector4& Color);
	void				SetCameraPosition(const vector3& Pos);
	void				SetViewProjection(const matrix44& VP);

	void				AddFrameShader(CStrID ID, PFrameShader FrameShader); //???or always load internally?
	void				SetScreenFrameShaderID(CStrID ID) { ScreenFrameShaderID = ID; }
	CFrameShader*		GetScreenFrameShader() const;

	//!!!must be static utility methods!
	//???to RenderFwd?
	EPixelFormat		GetPixelFormat(const CString& String); //???CStrID?
	int					GetFormatBits(EPixelFormat Format);

	DWORD				GetFeatureFlagSkinned() const { return FFlagSkinned; }
	DWORD				GetFeatureFlagInstanced() const { return FFlagInstanced; }

	void				SetWireframe(bool Wire);
	bool				IsWireframe() const { return Wireframe; }

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

inline void CRenderServer::AddFrameShader(CStrID ID, PFrameShader FrameShader)
{
	n_assert(ID.IsValid() && FrameShader.IsValid());
	FrameShaders.Add(ID, FrameShader);
}
//---------------------------------------------------------------------

inline CFrameShader* CRenderServer::GetScreenFrameShader() const
{
	int Idx = FrameShaders.FindIndex(ScreenFrameShaderID);
	return (Idx != INVALID_INDEX) ? FrameShaders.ValueAt(Idx).GetUnsafe() : NULL;
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
