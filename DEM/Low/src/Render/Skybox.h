#pragma once
#ifndef __DEM_L1_RENDER_SKYBOX_H__
#define __DEM_L1_RENDER_SKYBOX_H__

#include <Render/Renderable.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>

// Terrain represents a CDLOD heightmap-based model. It has special LOD handling
// and integrated visibility test.

namespace Resources
{
	typedef Ptr<class CResource> PResource;
}

namespace Render
{

class CSkybox: public IRenderable
{
	__DeclareClass(CSkybox);

protected:

	CStrID					MaterialUID;
	PMaterial				Material;
	PMesh					Mesh;

	virtual bool			ValidateResources(CGPUDriver* pGPU);

public:

	virtual bool			LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual IRenderable*	Clone();
	virtual bool			GetLocalAABB(CAABB& OutBox, UPTR LOD) const { OutBox = CAABB::Empty; OK; }	// Infinite size

	CMaterial*				GetMaterial() const { return Material.Get(); }
	CMesh*					GetMesh() const { return Mesh.Get(); }
};

typedef Ptr<CSkybox> PSkybox;

}

#endif
