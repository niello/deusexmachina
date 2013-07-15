#pragma once
#ifndef __DEM_L3_STORY_TASK_H__
#define __DEM_L3_STORY_TASK_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Task is an atomic part of the quest which can be activated and then completed or failed
// Task failure may or may not result in a whole quest failure

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Story
{

class CTask: public Core::CRefCounted
{
public:

	Scripting::PScriptObject	ScriptObj;

	// UI-related
	CString						Name;
	CString						Description;

	//CTask();
	//~CTask();

	//???succeed, fail here?
};

}

#endif
