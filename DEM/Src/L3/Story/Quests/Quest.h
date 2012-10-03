#pragma once
#ifndef __DEM_L3_STORY_QUEST_H__
#define __DEM_L3_STORY_QUEST_H__

#include <util/ndictionary.h>
#include "Task.h"

// Quest is a graph of linked tasks representing a part of storyline, either main or optional
// This implementation manages only current tasks. Links are managed by tasks themselves.

namespace Story
{

class CTask;

class CQuest: public Core::CRefCounted
{
public:

	enum EStatus
	{
		No,
		Opened,
		Done,
		Failed
	};

	struct CTaskRec
	{
		Ptr<CTask>	Task;
		EStatus		Status;
	};
	nDictionary<CStrID, CTaskRec>	Tasks;

	// UI-related
	nString							Name;
	nString							Description;

	//CQuest();
	//~CQuest();

	//bool	StartTask(CStrID ID);
	//bool	CompleteTask(CStrID ID);
	//bool	FailTask(CStrID ID);
};

}

#endif
