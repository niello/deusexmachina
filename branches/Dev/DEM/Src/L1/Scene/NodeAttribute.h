#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_H__
#define __DEM_L1_SCENE_NODE_ATTR_H__

#include <Core/Object.h>
#include <Data/Flags.h>
#include <Data/StringID.h>

// Scene node attributes implement specific logic, attached to 3D transform provided by scene nodes.
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

	friend class CSceneNode;

	enum
	{
		Active = 0x01	// Attribute must be processed
	};

	CSceneNode*		pNode;
	Data::CFlags	Flags;

	//???!!!fill and clear node here?!
	virtual bool	OnAttachToNode(CSceneNode* pSceneNode) { OK; }
	virtual void	OnDetachFromNode() { }

public:

	CNodeAttribute(): pNode(NULL), Flags(Active) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader) { FAIL; }
	virtual void	Update() = 0;
	void			RemoveFromNode();

	bool			IsAttachedToNode() const { return !!pNode; }
	CSceneNode*		GetNode() const { return pNode; }
	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool			IsActive() const { return Flags.Is(Active); }
};

typedef Ptr<CNodeAttribute> PNodeAttribute;

}

#endif
