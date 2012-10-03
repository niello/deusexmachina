#pragma once
#ifndef __DEM_L2_GFX_SHAPE_ENTITY_H__
#define __DEM_L2_GFX_SHAPE_ENTITY_H__

#include <Gfx/RenderableEntity.h>

// Entity that represents visible shape

namespace Graphics
{

class CShapeEntity: public CRenderableEntity
{
	DeclareRTTI;
	DeclareFactory(CShapeEntity);

protected:

	CSceneResource		ShadowResource;		// Optional
	bbox3				ShadowBBox;			// BB extruded to contain shadow
	uint				ShadowBoxFrameID;	// ...so that shadow Box is only computed once per frame

	nRenderContext		ShadowRenderCtx;

	virtual void		UpdateRenderContextVariables();
	void				ValidateShadowResource();

public:

	CShapeEntity();
	//virtual ~CShapeEntity();

	virtual void			Activate();
	virtual void			Deactivate();
	virtual void			Render();
	virtual void			RenderDebug();

	virtual EEntityType		GetType() const { return GFXShape; }
	
	const bbox3&			GetShadowBox();

	void					SetShadowResourceName(const nString& Name) { ShadowResource.Name = Name; }
	const nString&			GetShadowResourceName() const { return ShadowResource.Name; }
	const CSceneResource&	GetShadowResource() const { return ShadowResource; }
	void					SetRenderFlag(nRenderContext::Flag f, bool b);
};
//---------------------------------------------------------------------
    
typedef Ptr<CShapeEntity> PShapeEntity;

RegisterFactory(CShapeEntity);

inline void CShapeEntity::SetRenderFlag(nRenderContext::Flag f, bool b)
{
	RenderCtx.SetFlag(f, b);
	if (ShadowRenderCtx.IsValid()) ShadowRenderCtx.SetFlag(f, b);
}
//---------------------------------------------------------------------

}

#endif
