#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_H__
#define __DEM_L1_SCENE_NODE_ATTR_H__

#include <Core/Object.h>
#include <Data/Flags.h>
#include <Data/StringID.h>

// Scene node attributes implement specific logic, attached to a 3D transform provided by scene nodes.
// Common examples are meshes, lights, cameras etc.

namespace IO
{
	class CBinaryReader;
}

namespace Scene
{
class CSceneNode;

class CNodeAttribute: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	enum
	{
		Active				= 0x01,	// Attribute must be processed
		WorldMatrixChanged	= 0x02	// World matrix of the host node has changed, update of depended parts required
			//???!!!may be it is not so good to add this flag here, because different attributes use it differently and some
			//of them do not use at all. But doing it here is convinient.
			//!!!this flag is cleared by SPS update code. Maybe better is to have some event/callback OnWorldTfmChanged and
			//OnDimensionsChanged, and not rely on flag which state is managed by different parts in a counter-intuitive way!
	};

	CSceneNode*		pNode;
	Data::CFlags	Flags;

public:

	CNodeAttribute(): pNode(NULL), Flags(Active) {}

	virtual bool	OnAttachToNode(CSceneNode* pSceneNode) { if (pNode) FAIL; pNode = pSceneNode; Flags.Set(WorldMatrixChanged); return !!pNode; }
	virtual void	OnDetachFromNode() { pNode = NULL; }

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader) { FAIL; }
	virtual void	Update(const vector3* pCOIArray, DWORD COICount);
	void			RemoveFromNode();

	bool			IsAttachedToNode() const { return !!pNode; }
	CSceneNode*		GetNode() const { return pNode; }
	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool			IsActive() const { return Flags.Is(Active); }
};

typedef Ptr<CNodeAttribute> PNodeAttribute;

}

#endif
