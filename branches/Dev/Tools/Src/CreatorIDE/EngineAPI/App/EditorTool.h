#pragma once
#ifndef __CIDE_EDITOR_TOOL_H__
#define __CIDE_EDITOR_TOOL_H__

#include <Core/RTTI.h>

// Editor tool interface. Tools are used for input processing and rendering control
// in different editor modes, like Selection, Transform, NavRegion.

namespace App
{

class IEditorTool
{
	//DeclareRTTI;

public:

	virtual ~IEditorTool() {}

	virtual void Activate() = 0;
	virtual void Deactivate() = 0;
	//virtual void Init(state) = 0;
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
