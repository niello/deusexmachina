#pragma once
#include <Render/Renderable.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>

// Model represents a piece of visible polygonal geometry.
// It ties together a mesh group, a material and per-object rendering params.

class matrix44;

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
	const matrix44*                pSkinPalette = nullptr; // nullptr if no skin
	U32                            BoneCount = 0;
	U32                            ShaderTechIndex = INVALID_INDEX_T<U32>;
};

}
