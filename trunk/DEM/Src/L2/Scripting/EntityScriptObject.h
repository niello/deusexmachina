#pragma once
#ifndef __DEM_L2_ENTITY_SCRIPT_OBJ_H__
#define __DEM_L2_ENTITY_SCRIPT_OBJ_H__

#include <Scripting/ScriptObject.h>

//#undef RegisterClass

// Entity script object is bound to some game entity instance.

namespace Game
{
	class CEntity;
}

namespace Scripting
{

class CEntityScriptObject: public CScriptObject
{
	DeclareRTTI;
	DeclareFactory(CEntityScriptObject);

protected:

	Game::CEntity*	pEntity; //???Ptr? or will cause cyclic reference?

	CEntityScriptObject(): pEntity(NULL) {}

public:

	friend class CScriptServer; //???why protected constructor?

	CEntityScriptObject(Game::CEntity& Ent, LPCSTR ObjName, LPCSTR TableName = NULL):
		CScriptObject(ObjName, TableName), pEntity(&Ent) {}
	//virtual ~CEntityScriptObject();

	static bool		RegisterClass();

	virtual int		GetField(LPCSTR Key) const;
	virtual bool	SetField(LPCSTR Key, const CData& Value);

	bool			SubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority = Priority_Default);
	void			UnsubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName);

	void			SetEntity(Game::CEntity* pEnt) { pEntity = pEnt; }
	Game::CEntity*	GetEntity() const { return pEntity; }
};

RegisterFactory(CEntityScriptObject);

typedef Ptr<CEntityScriptObject> PEntityScriptObject;

}

#endif
