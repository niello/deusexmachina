#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Render/ShaderParamTable.h>
#include <map>
#include <vector>

// Render path incapsulates a full algorithm to render a frame, allowing to
// define it in a data-driven manner and therefore to avoid hardcoding frame rendering.
// It describes, how to use what shaders on what objects. The final output
// is a complete frame, rendered to an output render target.
// Render path consists of phases, each of which fills RT, MRT and/or DS,
// or does some intermediate processing, like an occlusion culling.
// Render path could be designed for some feature level (DX9, DX11), for some
// rendering concept (forward, deferred), for different features used (HDR) etc.

namespace Data
{
	class CParams;
}

namespace Frame
{
class CView;
typedef Ptr<class CRenderPhase> PRenderPhase;

class CRenderPath: public Core::CObject
{
	RTTI_CLASS_DECL(Frame::CRenderPath, Core::CObject);

protected:

	struct CRenderTargetSlot
	{
		vector4	ClearValue;
	};

	struct CDepthStencilSlot
	{
		U32		ClearFlags;
		float	DepthClearValue;
		U8		StencilClearValue;
	};

	std::map<CStrID, CRenderTargetSlot> RTSlots;
	std::map<CStrID, CDepthStencilSlot> DSSlots;

public:

	// FIXME: for loading
	std::vector<PRenderPhase>           Phases;
	Render::PShaderParamTable           Globals;
	Render::CShaderConstantParam        ViewProjection;
	Render::CShaderConstantParam        CameraPosition;
	std::vector<std::map<Render::EEffectType, CStrID>> EffectOverrides;
	std::map<CStrID, U32>               _RenderQueues;

	CRenderPath();
	virtual ~CRenderPath() override;

	void AddRenderTargetSlot(CStrID ID, vector4 ClearValue);
	void AddDepthStencilSlot(CStrID ID, U32 ClearFlags, float DepthClearValue, U8 StencilClearValue);

	bool                       HasRenderTarget(CStrID ID) const { return RTSlots.find(ID) != RTSlots.cend(); }
	bool                       HasDepthStencilBuffer(CStrID ID) const { return DSSlots.find(ID) != DSSlots.cend(); }
	void                       SetRenderTargetClearColor(CStrID ID, const vector4& Color);

	bool                       Render(CView& View);

	UPTR                       GetRenderTargetCount() const { return RTSlots.size(); }
	UPTR                       GetDepthStencilBufferCount() const { return DSSlots.size(); }
	Render::CShaderParamTable& GetGlobalParamTable() const { return *Globals; }
};

typedef Ptr<CRenderPath> PRenderPath;

}
