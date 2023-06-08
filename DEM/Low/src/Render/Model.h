#pragma once
#include <Render/Renderable.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>

// Model represents a piece of visible polygonal geometry.
// It ties together a mesh group, a material and per-object rendering params.

namespace Render
{
using PMesh = Ptr<class CMesh>;
using PMaterial = Ptr<class CMaterial>;
struct CPrimitiveGroup;

class CModel : public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PMaterial                      Material; //???!!!materialset!?
	PMesh                          Mesh;
	const Render::CPrimitiveGroup* pGroup = nullptr;

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling (Skinned, EnableInstancing etc too?)
};

}
