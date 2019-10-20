#pragma once
#include <Render/RenderFwd.h>

// Manages all GPU-dependent resources required for frame rendering,
// prevents redundant GPU resource loading

namespace Resources
{
	class CResourceManager;
}

namespace Frame
{
typedef Ptr<class CRenderPath> PRenderPath;

class CFrameResourceManager final
{
private:

	Resources::CResourceManager* pResMgr = nullptr;
	Render::CGPUDriver* pGPU = nullptr;

	std::unordered_map<CStrID, Render::PMesh>		Meshes;
	std::unordered_map<CStrID, Render::PTexture>	Textures;
	std::unordered_map<CStrID, Render::PShader>		Shaders;
	std::unordered_map<CStrID, Render::PEffect>		Effects;
	std::unordered_map<CStrID, Render::PMaterial>	Materials;
	std::unordered_map<CStrID, PRenderPath>			RenderPaths;

public:

	CFrameResourceManager(Resources::CResourceManager& ResMgr, Render::CGPUDriver& GPU);

	// Engine resource management - create GPU (VRAM) resource from engine resource
	//???!!!resolve assigns?!
	Render::PMesh		GetMesh(CStrID UID);
	Render::PTexture	GetTexture(CStrID UID, UPTR AccessFlags);
	Render::PShader		GetShader(CStrID UID);
	Render::PEffect		GetEffect(CStrID UID);
	Render::PMaterial	GetMaterial(CStrID UID);
	PRenderPath			GetRenderPath(CStrID ID);

	//???CreateView here? or CView constructor is enough?

	//???temporary CBs?
	//???render node pool?
};

}
