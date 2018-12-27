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

	virtual bool InternalActivate();
	virtual void InternalDeactivate();

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);

	//???!!!listen OnEntityRenamed?!

public:

	//CPropScriptable() {}
	//virtual ~CPropScriptable();

	//???or hide & add RunFunction calls here?
	PEntityScriptObject GetScriptObject() const { return Obj; }
};

}

#endif