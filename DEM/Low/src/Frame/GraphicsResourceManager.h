#pragma once
#include <Render/RenderFwd.h>
#include <Data/RefCounted.h>
#include <map>

// Manages all GPU-dependent resources required for frame rendering,
// prevents redundant GPU resource loading

namespace IO
{
	class CBinaryReader;
}

namespace Resources
{
	class CResourceManager;
}

namespace Render
{
	struct CShaderParamValues;
}

namespace Frame
{
typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
typedef Ptr<class CRenderPath> PRenderPath;

class CGraphicsResourceManager : public Data::CRefCounted
{
private:

	Resources::CResourceManager* pResMgr = nullptr; //???strong ref?
	Render::CGPUDriver* pGPU = nullptr; //???strong ref?

	std::unordered_map<CStrID, Render::PMesh>     Meshes;
	std::unordered_map<CStrID, Render::PTexture>  Textures;
	std::unordered_map<CStrID, Render::PShader>   Shaders;
	std::unordered_map<CStrID, Render::PEffect>   Effects;
	std::unordered_map<CStrID, Render::PMaterial> Materials;
	std::unordered_map<CStrID, PRenderPath>       RenderPaths;

	bool LoadShaderParamValues(IO::CBinaryReader& Reader, Render::CShaderParamValues& Out);
	bool LoadRenderStateDesc(IO::CBinaryReader& Reader, Render::CRenderStateDesc& Out, bool LoadParamTables);

	Render::PEffect           LoadEffect(CStrID UID);
	Render::PMaterial         LoadMaterial(CStrID UID);
	PRenderPath               LoadRenderPath(CStrID UID);

public:

	CGraphicsResourceManager(Resources::CResourceManager& ResMgr, Render::CGPUDriver& GPU);
	virtual ~CGraphicsResourceManager() override;

	// Engine resource management - create GPU (VRAM) resource from engine resource
	//???!!!resolve assigns?!
	Render::PMesh     GetMesh(CStrID UID);
	Render::PTexture  GetTexture(CStrID UID, UPTR AccessFlags);
	Render::PShader   GetShader(CStrID UID, bool NeedParamTable);
	Render::PEffect   GetEffect(CStrID UID);
	Render::PMaterial GetMaterial(CStrID UID);
	PRenderPath       GetRenderPath(CStrID ID);

	//???CreateView here? or CView constructor is enough?

	//???temporary CBs?
	//???render node pool?

	Resources::CResourceManager* GetResourceManager() const { return pResMgr; }
	Render::CGPUDriver*          GetGPU() const { return pGPU; }
};

}