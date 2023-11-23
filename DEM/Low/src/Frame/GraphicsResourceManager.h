#pragma once
#include <Render/RenderFwd.h>
#include <Data/StringID.h>
#include <Data/RefCounted.h>

// Manages all GPU-dependent resources required for frame rendering, prevents redundant GPU resource loading.
// Objects created by GPU are specific to that GPU, so each GPU manages its own textures, shaders etc.
// They can't be shared through a CResourceManager, but it may manage templates, like RAM texture data
// or material template with IDs instead of real GPU objects. Template can be instantiated with a GPU here.

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

namespace Data
{
	class CParams;
}

namespace UI
{
	class CUIServer;
}

namespace Frame
{
typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
typedef Ptr<class CRenderPath> PRenderPath;
typedef std::unique_ptr<class CView> PView;

class CGraphicsResourceManager : public Data::CRefCounted
{
private:

	Resources::CResourceManager*                  pResMgr = nullptr; //???strong ref?
	Render::PGPUDriver                            GPU;
	std::unique_ptr<UI::CUIServer>                UIServer; // FIXME: is the right place?

	std::unordered_map<CStrID, Render::PMesh>     Meshes;
	std::unordered_map<CStrID, Render::PTexture>  Textures;
	std::unordered_map<CStrID, Render::PShader>   Shaders;
	std::unordered_map<CStrID, Render::PEffect>   Effects;
	std::unordered_map<CStrID, Render::PMaterial> Materials;
	std::unordered_map<CStrID, PRenderPath>       RenderPaths;
	std::unordered_map<CStrID, U16>               _MaterialKeyCounters; // Per effect

	U16 _MeshKeyCounter = 1;
	U8  _TechKeyCounter = 1;

	bool LoadShaderParamValues(IO::CBinaryReader& Reader, const Render::CShaderParamTable& MaterialTable, Render::CShaderParamValues& Out);
	bool LoadRenderStateDesc(IO::CBinaryReader& Reader, Render::CRenderStateDesc& Out, bool LoadParamTables);

	Render::PEffect LoadEffect(CStrID UID);
	Render::PMaterial LoadMaterial(CStrID UID);
	PRenderPath LoadRenderPath(CStrID UID);

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

	bool              InitUI(const Data::CParams* pSettings = nullptr);

	PView             CreateView(CStrID RenderPathID, int SwapChainID = INVALID_INDEX, CStrID SwapChainRenderTargetID = CStrID::Empty);

	void              Update(float dt);

	Resources::CResourceManager* GetResourceManager() const { return pResMgr; }
	Render::CGPUDriver*          GetGPU() const;
	UI::CUIServer*               GetUI() const { return UIServer.get(); }
};

}
