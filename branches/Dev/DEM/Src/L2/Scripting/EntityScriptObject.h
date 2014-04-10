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
	__DeclareClass(CEntityScriptObject);

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
	virtual bool	SetField(LPCSTR Key, const Data::CData& Value);

	bool			SubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority = Events::Priority_Default);
	void			UnsubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName);

	void			SetEntity(Game::CEntity* pEnt) { pEntity = pEnt; }
	Game::CEntity*	GetEntity() const { return pEntity; }
};

typedef Ptr<CEntityScriptObject> PEntityScriptObject;

}

#define SETUP_ENT_SI_ARGS(MinArgumentCount) \
	n_assert2_dbg(lua_istable(l, 1), "Entity script call argument 1 is not a table, make sure : and not . is used for the call!"); \
	int ArgCount = lua_gettop(l);		\
	if (ArgCount < MinArgumentCount)		\
	{									\
		lua_settop(l, 0);				\
		n_printf(__FUNCTION__ " > %d argument(s) passed while at least %d required\n", ArgCount - 1, MinArgumentCount - 1); \
		return 0;						\
	}									\
	Scripting::CEntityScriptObject* This = (Scripting::CEntityScriptObject*)CScriptObject::GetFromStack(l, 1);	\
	n_assert2_dbg(This, "Entity script call doesn't specify CEntityScriptObject This, make sure : and not . is used for the call!"); \
	if (!This) return 0;

#endif
