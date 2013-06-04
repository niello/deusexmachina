#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_H__
#define __DEM_L1_SCENE_NODE_ATTR_H__

#include <Core/RefCounted.h>
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
class CScene;
class CSceneNode;

class CNodeAttribute: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	enum
	{
		Active = 0x01	// Attribute must be processed
	};

	friend class CSceneNode;

	CSceneNode*		pNode;

	//???Active flag?
	Data::CFlags	Flags; //???or in children only?

public:

	CNodeAttribute(): pNode(NULL), Flags(Active) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, IO::CBinaryReader& DataReader) { FAIL; }
	virtual bool	OnAdd() { OK; }
	virtual void	OnRemove() {}
	virtual void	Update() = 0;
	void			RemoveFromNode();

	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool			IsActive() const { return Flags.Is(Active); }
	CSceneNode*		GetNode() const { return pNode; }
};

typedef Ptr<CNodeAttribute> PNodeAttribute;

}

#endif
