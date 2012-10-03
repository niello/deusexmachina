#pragma once
#ifndef __DEM_L2_GFX_RENDERABLE_ENTITY_H__
#define __DEM_L2_GFX_RENDERABLE_ENTITY_H__

#include <Gfx/Entity.h>
#include <Gfx/SceneResource.h>
#include <scene/nrendercontext.h>

// An entity that can be rendered (light or mesh)

namespace Graphics
{

class CRenderableEntity: public CEntity
{
	DeclareRTTI;

protected:

	CSceneResource		Resource;

	nVariable::Handle	TimeVarHandle;
	nVariable::Handle	OneVarHandle;	// constant 1.0 channel
	nVariable::Handle	WindVarHandle;

	nRenderContext		RenderCtx;

	virtual void		UpdateGlobalBox();
	virtual void		UpdateRenderContextVariables();
	void				ValidateResource();
	void				RenderRenderContext(nRenderContext& Ctx);

public:

	CRenderableEntity();

	virtual void			Activate();
	virtual void			Deactivate();
	virtual void			OnRenderBefore(); //???event?
	virtual void			Render() = 0;

	void					SetVisible(bool Visible) { Flags.SetTo(VISIBLE, Visible); }
	bool					GetVisible() const { return Flags.Is(VISIBLE); }

	void					SetResourceName(const nString& Name) { Resource.Name = Name; }
	const nString&			GetResourceName() const { return Resource.Name; }
	const CSceneResource&	GetResource() const { return Resource; }
	nRenderContext&			GetRenderContext() { return RenderCtx; }
	
	// For CLinkedListSet
	virtual EEntityType	GetKey() const { return GFXShape; }
};

}

#endif
