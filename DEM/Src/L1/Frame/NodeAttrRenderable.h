#pragma once
#ifndef __DEM_L1_FRAME_NODE_ATTR_RENDERABLE_H__
#define __DEM_L1_FRAME_NODE_ATTR_RENDERABLE_H__

#include <Scene/NodeAttribute.h>

// Base attribute class for any renderable scene objects.

class CAABB;

namespace Scene
{
	class CSPS;
	struct CSPSRecord;
}

namespace Render
{
	class IRenderable;
}

namespace Frame
{

class CNodeAttrRenderable: public Scene::CNodeAttribute
{
	__DeclareClass(CNodeAttrRenderable);

protected:

	Render::IRenderable*	pRenderable; //!!!???PRenderable?!
	Scene::CSPS*			pSPS;
	Scene::CSPSRecord*		pSPSRecord;

	virtual void			OnDetachFromNode();

public:

	CNodeAttrRenderable(): pRenderable(NULL), pSPS(NULL), pSPSRecord(NULL) {}
	~CNodeAttrRenderable();

	virtual bool					LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual Scene::PNodeAttribute	Clone();
	void							UpdateInSPS(Scene::CSPS& SPS);

	bool							GetGlobalAABB(CAABB& OutBox, UPTR LOD = 0) const; //!!!can get from a spatial record!
	Render::IRenderable*			GetRenderable() const { return pRenderable; }
};

typedef Ptr<CNodeAttrRenderable> PNodeAttrRenderable;

}

#endif
