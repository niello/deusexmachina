#pragma once
#ifndef __DEM_L2_PROP_SCRIPTABLE_H__
#define __DEM_L2_PROP_SCRIPTABLE_H__

#include <Game/Property.h>
#include <Scripting/EntityScriptObject.h>

// This property adds script object to the entity and manages entity script interface

namespace Prop
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