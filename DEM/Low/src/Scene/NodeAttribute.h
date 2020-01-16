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

	friend class CSceneNode;

	// First 4 bits are reserved, children may use ones from 0x10
	enum
	{
		SelfActive        = 0x01, // Attribute itself is active
		EffectivelyActive = 0x02, // Attribute is effectively active, meaning it is active and attached to an active node
	};

	CSceneNode*  _pNode = nullptr;
	Data::CFlags _Flags;

	virtual void            UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount) {}
	virtual void            UpdateAfterChildren(const vector3* pCOIArray, UPTR COICount) {}

	void                    SetNode(CSceneNode* pNode) { _pNode = pNode; UpdateActivity(); }
	void                    UpdateActivity();
	virtual void            OnActivityChanged(bool Active) {}

public:

	CNodeAttribute() : _Flags(SelfActive) {}

	virtual bool            LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) { OK; }
	virtual bool            ValidateResources(Resources::CResourceManager& ResMgr) { OK; }
	virtual void            RenderDebug(Debug::CDebugDraw& DebugDraw) const {}

	virtual PNodeAttribute  Clone() = 0;
	void                    RemoveFromNode();

	CSceneNode*            	GetNode() const { return _pNode; }
	bool                    IsActiveSelf() const { return _Flags.Is(SelfActive); }
	bool                    IsActive() const { return _Flags.Is(EffectivelyActive); }
	void                    SetActive(bool Enable) { _Flags.SetTo(SelfActive, Enable); UpdateActivity(); }
};

}
