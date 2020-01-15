#pragma once
#include <Core/Object.h>
#include <Data/Flags.h>
#include <Data/StringID.h>
#include <Math/Vector3.h>

// Scene node attributes implement specific logic, attached to a 3D transform provided by scene nodes.
// Common examples are meshes, lights, cameras etc.

namespace IO
{
	class CBinaryReader;
}

namespace Resources
{
	class CResourceManager;
}

namespace Debug
{
	class CDebugDraw;
}

namespace Scene
{
class CSceneNode;
typedef Ptr<class CNodeAttribute> PNodeAttribute;

class CNodeAttribute: public Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	enum
	{
		Active = 0x01,
	};

	CSceneNode*  pNode = nullptr;
	Data::CFlags Flags;

public:

	CNodeAttribute() : Flags(Active) {}

	virtual bool			OnAttachToNode(CSceneNode* pSceneNode) { if (pNode) FAIL; pNode = pSceneNode; return !!pNode; }
	virtual void			OnDetachFromNode() { pNode = nullptr; }
	virtual void			OnDetachFromScene() {}

	virtual bool			LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) { OK; }
	virtual bool            ValidateResources(Resources::CResourceManager& ResMgr) { OK; }
	virtual PNodeAttribute	Clone() = 0;
	virtual void			UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount) {}
	virtual void			UpdateAfterChildren(const vector3* pCOIArray, UPTR COICount) {}
	virtual void            RenderDebug(Debug::CDebugDraw& DebugDraw) const {}
	void					RemoveFromNode();

	CSceneNode*				GetNode() const { return pNode; }
	void					SetActive(bool Enable) { return Flags.SetTo(Active, Enable); }
	bool					IsActive() const { return Flags.Is(Active); }
};

}
