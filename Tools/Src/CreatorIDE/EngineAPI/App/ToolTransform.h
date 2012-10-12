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

	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);

public:

	//CToolTransform();
	//virtual ~CToolTransform() {}

	virtual void Activate();
	virtual void Deactivate();
	virtual void Render();
};

}

#endif
