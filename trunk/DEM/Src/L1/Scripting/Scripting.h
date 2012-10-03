#pragma once
#ifndef __DEM_L1_SCRIPTING_H__
#define __DEM_L1_SCRIPTING_H__

// Scripting system declarations

#define SETUP_ENT_SI_ARGS(ArgumentCount) \
	int ArgCount = lua_gettop(l);		\
	if (ArgCount < ArgumentCount)		\
	{									\
		lua_settop(l, 0);				\
		return 0;						\
	}									\
	CEntityScriptObject* This = (CEntityScriptObject*)CScriptObject::GetFromStack(l, 1);	\
	if (!This) return 0;

#endif
