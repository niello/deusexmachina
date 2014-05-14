#pragma once
#ifndef __DEM_L1_SCENE_NODE_CTLR_H__
#define __DEM_L1_SCENE_NODE_CTLR_H__

#include <Core/Object.h>
#include <Scene/SceneFwd.h>
#include <Data/Flags.h>
#include <Math/TransformSRT.h>

// Scene node controller provides transform (SRT) parameters. It is mainly used for
// scene node animation. Animation controller subclasses can sample keyframed animation
// data, use physics as a transformation source or implement any other custom logic,
// such as look-at or even constant value. Animation controller can also implement
// selection or blending algorithm using other animation controllers as inputs.

//???LoadDataBlock, as in node attrs?

namespace Scene
{
class CNodeControllerComposite;

class CNodeController: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	friend class CSceneNode;

	enum
	{
		Active				= 0x01,	// Controller must be processed
		LocalSpace			= 0x02,	// Controller produces local-space transform
		UpdateLocalSpace	= 0x04	// Controller wants the host to update local transform from provided world one
	};

	CSceneNode*		pNode;
	Data::CFlags	Flags;
	Data::CFlags	Channels;

public:

	CNodeController(): pNode(NULL) {}

	virtual bool	OnAttachToNode(CSceneNode* pSceneNode) { pNode = pSceneNode; OK; }
	virtual void	OnDetachFromNode() { pNode = NULL; }
	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm) = 0;
	void			RemoveFromNode();

	bool			IsAttachedToNode() const { return !!pNode; }
	CSceneNode*		GetNode() const { return pNode; }
	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool			IsActive() const { return Flags.Is(Active); }
	void			SetLocalSpace(bool Local) { Flags.SetTo(LocalSpace, Local); }
	bool			IsLocalSpace() const { return Flags.Is(LocalSpace); }
	bool			NeedToUpdateLocalSpace() const { return Flags.Is(UpdateLocalSpace); }
	bool			HasChannel(EChannel Channel) const { return Channels.Is(Channel); }
	DWORD			GetChannels() const { return Channels.GetMask(); }
};

typedef Ptr<CNodeController> PNodeController;

}

#endif
