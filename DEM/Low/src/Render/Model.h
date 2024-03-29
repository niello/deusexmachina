#pragma once
#include <Render/Renderable.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <map>

// Model represents a piece of visible polygonal geometry.
// It ties together a mesh group, a material and per-object rendering params.

namespace Render
{
using PMesh = Ptr<class CMesh>;
using PMaterial = Ptr<class CMaterial>;
struct CPrimitiveGroup;
class CLight;

class CModel : public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PMaterial                      Material; //???!!!materialset!?
	PMesh                          Mesh;
	const Render::CPrimitiveGroup* pGroup = nullptr;
	const rtm::matrix3x4f*         pSkinPalette = nullptr; // nullptr if no skin
	std::map<UPTR, CLight*>        Lights;
	U32                            BoneCount = 0;
	U32                            ShaderTechIndex = INVALID_INDEX_T<U32>;
};

}
