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

	Resources::PResource	RMaterial;
	PMaterial				Material;	//???NEED?
	PMesh					Mesh;		//???NEED?

	virtual bool			ValidateResources(CGPUDriver* pGPU);

public:

	virtual bool			LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual IRenderable*	Clone();
	virtual bool			GetLocalAABB(CAABB& OutBox, UPTR LOD) const { OutBox = CAABB::Empty; OK; }	// Infinite size

	CMaterial*				GetMaterial() const { return Material.GetUnsafe(); }
	CMesh*					GetMesh() const { return Mesh.GetUnsafe(); }
};

typedef Ptr<CSkybox> PSkybox;

}

#endif
