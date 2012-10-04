#pragma once
#ifndef __DEM_L2_PROP_PLR_CHAR_INPUT_H__ //???!!!L3?!
#define __DEM_L2_PROP_PLR_CHAR_INPUT_H__

#include <Input/Prop/PropInput.h>

// Player character input - movement & IAO interactions

namespace Input
{
	enum EMouseButton;
}

namespace Properties
{

class CPropPlrCharacterInput: public CPropInput
{
	DeclareRTTI;
	DeclareFactory(CPropPlrCharacterInput);

protected:

	virtual void	ActivateInput();
    virtual void	DeactivateInput();
	bool			OnMouseClick(Input::EMouseButton Button, bool Double = false);

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnMouseBtnUp);
	DECLARE_EVENT_HANDLER(MouseDoubleClick, OnMouseDoubleClick);
	DECLARE_EVENT_HANDLER(MouseMoveRaw, OnMouseMoveRaw);
	DECLARE_EVENT_HANDLER(MouseWheel, OnMouseWheel);

public:

};

RegisterFactory(CPropPlrCharacterInput);

}

#endif