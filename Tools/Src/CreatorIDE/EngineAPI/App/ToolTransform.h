#pragma once
#ifndef __CIDE_EDITOR_TOOL_TRANSFORM_H__
#define __CIDE_EDITOR_TOOL_TRANSFORM_H__

#include <App/EditorTool.h>
#include <Events/Events.h>

// Transform tool. Converts user input to transform changes applied on the selected entity.
// In this implementation this tool doesn't support multiselection.

namespace App
{

class CToolTransform: public IEditorTool
{
	//DeclareRTTI;

protected:

	// bool MultiSelect;
	// mouse click handlers to perform entity selection
	//!!!list of selected entities is external (state or app)!

	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);

public:

	//CToolTransform();
	//virtual ~CToolTransform() {}

	virtual void Activate();
	virtual void Deactivate();
	virtual void Render();
	//virtual void Init(class Sample* sample) = 0;
	//virtual void Reset() = 0;
	//virtual void HandleMenu() = 0;
	//virtual void HandleClick(const float* s, const float* p, bool shift) = 0;
	//virtual void HandleToggle() = 0;
	//virtual void HandleStep() = 0;
	//virtual void HandleUpdate(const float dt) = 0;
};

}

#endif
