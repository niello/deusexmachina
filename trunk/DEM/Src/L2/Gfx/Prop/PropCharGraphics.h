#pragma once
#ifndef __DEM_L2_PROP_CHAR_GRAPHICS_H__
#define __DEM_L2_PROP_CHAR_GRAPHICS_H__

#include <Gfx/Prop/PropGraphics.h>
#include <Gfx/CharEntity.h>
#include <DB/AttrID.h>

// A specialized graphics property for skinned characters. This creates a
// Graphics::CCharEntity, that knows how to switch animations and manages attachments.
// Based on mangalore ActorGraphicsProperty_ (C) 2005 Radon Labs GmbH

namespace Attr
{
	DeclareString(AnimSet);
	DeclareString(CharacterSet);	// Name of the character 3 charset
}

namespace Properties
{

class CPropCharGraphics: public CPropGraphics
{
	DeclareRTTI;
	DeclareFactory(CPropCharGraphics);

protected:

	struct Attachment
	{
		int							JointIndex;
		Ptr<Graphics::CShapeEntity>	GraphicsEntity;
		matrix44					OffsetMatrix;
		bool						NewCreated;
	};

	nArray<Attachment> Attachments; //???!!!entities to gfx ent array?!
	
	virtual void SetupGraphicsEntities();

	DECLARE_EVENT_HANDLER(GfxSetAnimation, OnGfxSetAnimation);
	DECLARE_EVENT_HANDLER(GfxAddAttachment, OnGfxAddAttachment);
	DECLARE_EVENT_HANDLER(GfxRemoveAttachment, OnGfxRemoveAttachment);
	DECLARE_EVENT_HANDLER(OnRender, OnRender);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);

	virtual void UpdateTransform(); 

	void	RemoveAttachment(const nString& JointName);
	int		FindAttachment(const nString& JointName);
	void	UpdateAttachments();

	// Character3 stuff
	// Let before understand what is it at all
	//bool	OnGfxAddSkin(const nString& SkinName);
	//bool	OnGfxRemoveSkin(const nString& SkinName);
	//bool	OnGfxSetCharacterSet(const nString& CharacterSetName);

public:

	CPropCharGraphics() {}
	virtual ~CPropCharGraphics();

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void Activate();
	virtual void Deactivate();

	Graphics::CCharEntity* GetGraphicsEntity() const;
};
//---------------------------------------------------------------------

RegisterFactory(CPropCharGraphics);

inline Graphics::CCharEntity* CPropCharGraphics::GetGraphicsEntity() const
{
	n_assert(GraphicsEntities[0]->IsA(Graphics::CCharEntity::RTTI));
	return (Graphics::CCharEntity*)GraphicsEntities[0].get();
}
//---------------------------------------------------------------------

} // namespace Properties

#endif
