#pragma once
#ifndef __DEM_L2_PROP_SCRIPTABLE_H__
#define __DEM_L2_PROP_SCRIPTABLE_H__

#include <Game/Property.h>
#include <Scripting/EntityScriptObject.h>
#include <DB/AttrID.h>

// This property adds script object to the entity and manages entity script interface

namespace Attr
{
	DeclareString(ScriptClass);	// Lua class of hosted scriptable object
	DeclareString(Script);		// Custom lua script to load directly into object
};

namespace Properties
{
using namespace Scripting;

class CPropScriptable: public Game::CProperty
{
	__DeclareClass(CPropScriptable);
	__DeclarePropertyStorage;

protected:

	PEntityScriptObject Obj;

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnLoad, OnLoad);
	DECLARE_EVENT_HANDLER(OnSave, OnSave);
	DECLARE_EVENT_HANDLER(OnDelete, OnDelete);

	//???!!!listen OnEntityRenamed?!

public:

	//CPropScriptable() {}
	//virtual ~CPropScriptable();

	virtual void	Activate();
	virtual void	Deactivate();

	//???or hide & add RunFunction calls here?
	PEntityScriptObject GetScriptObject() const { return Obj; }
};

}

#endif