#pragma once
#ifndef __DEM_L2_EVENT_QUEUE_TASK_H__
#define __DEM_L2_EVENT_QUEUE_TASK_H__

#include <Events/EventNative.h>
#include <AI/Behaviour/Task.h>

// Queues a new task into an actor's task queue

namespace Event
{

class QueueTask: public Events::CEventNative
{
	__DeclareClass(QueueTask);

public:

	AI::PTask Task;

	QueueTask(AI::CTask* pTask = NULL): Task(pTask) {}
};

}

#endif
