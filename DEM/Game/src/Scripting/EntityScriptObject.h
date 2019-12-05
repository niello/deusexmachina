#pragma once
#include <Scripting/ScriptObject.h>

// Entity script object is bound to some game entity instance.

namespace Game
{
	class CEntity;
}

namespace Scripting
{

class CEntityScriptObject: public CScriptObject
{
	FACTORY_CLASS_DECL;

protected:

	Game::CEntity*	pEntity; //???Ptr? or will cause cyclic reference?

	CEntityScriptObject(): pEntity(nullptr) {}

public:

	friend class CScriptServer; //???why protected constructor?

	CEntityScriptObject(Game::CEntity& Ent, const char* ObjName, const char* TableName = nullptr):
		CScriptObject(ObjName, TableName), pEntity(&Ent) {}
	//virtual ~CEntityScriptObject();

	static bool		RegisterClass();

	virtual int		GetField(const char* Key) const;
	virtual bool	SetField(const char* Key, const Data::CData& Value);

	bool			SubscribeLocalEvent(CStrID EventID, const char* HandlerFuncName, U16 Priority = Events::Priority_Default);
	void			UnsubscribeLocalEvent(CStrID EventID, const char* HandlerFuncName);

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
		Sys::Log(__FUNCTION__ " > %d argument(s) passed while at least %d required\n", ArgCount - 1, MinArgumentCount - 1); \
		return 0;						\
	}									\
	Scripting::CEntityScriptObject* This = (Scripting::CEntityScriptObject*)CScriptObject::GetFromStack(l, 1);	\
	n_assert2_dbg(This, "Entity script call doesn't specify CEntityScriptObject This, make sure : and not . is used for the call!"); \
	if (!This) return 0;
