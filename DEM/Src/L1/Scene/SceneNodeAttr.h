#pragma once
#ifndef __DEM_L1_SCENE_NODE_ATTR_H__
#define __DEM_L1_SCENE_NODE_ATTR_H__

#include <Core/RefCounted.h>
#include <Data/Flags.h>
#include <Data/StringID.h>

// Scene node attributes implement specific logic, attached to 3D transform provided by scene nodes.
// Common examples are meshes, lights, cameras etc.

namespace Data
{
	class CBinaryReader;
}

namespace Scene
{
class CScene;
class CSceneNode;

class CSceneNodeAttr: public Core::CRefCounted
{
	DeclareRTTI;

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

	CSceneNodeAttr(): pNode(NULL), Flags(Active) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader) { FAIL; }
	virtual void	Update(CScene& Scene) = 0;
	virtual void	PrepareToRender() {}

	bool			IsActive() const { return Flags.Is(Active); }
	void			Activate(bool Enable) { return Flags.SetTo(Active, Enable); }
	CSceneNode*		GetNode() const { return pNode; }
};

typedef Ptr<CSceneNodeAttr> PSceneNodeAttr;

}

#endif
