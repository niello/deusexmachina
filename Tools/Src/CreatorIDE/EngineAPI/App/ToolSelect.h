#pragma once
#ifndef __CIDE_EDITOR_TOOL_SELECT_H__
#define __CIDE_EDITOR_TOOL_SELECT_H__

#include <App/EditorTool.h>
#include <Events/Events.h>

// Selection tool. Just selects one (or more?) entities for further interaction.
// Application or this tool should update UI to show selection properties and
// optionally render selection.

namespace App
{

class CToolSelect: public IEditorTool
{
	//DeclareRTTI;

protected:

	// bool MultiSelect;
	// mouse click handlers to perform entity selection
	//!!!list of selected entities is external (state or app)!

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);

public:

	//CToolSelect();
	//virtual ~CToolSelect() {}

	virtual void Activate();
	virtual void Deactivate();
	//virtual void Init(class Sample* sample) = 0;
	//virtual void Reset() = 0;
	//virtual void HandleMenu() = 0;
	//virtual void HandleClick(const float* s, const float* p, bool shift) = 0;
	//virtual void HandleRender() = 0;
	//virtual void HandleToggle() = 0;
	//virtual void HandleStep() = 0;
	//virtual void HandleUpdate(const float dt) = 0;
};

}

#endif
