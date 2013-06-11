#pragma once
#ifndef __DEM_L2_PROP_PLR_CHAR_INPUT_H__ //???!!!L3?!
#define __DEM_L2_PROP_PLR_CHAR_INPUT_H__

#include <Game/Property.h>

// Player character input - movement & IAO interactions

namespace Input
{
	enum EMouseButton;
}

namespace Prop
{

class CPropPlrCharacterInput: public Game::CProperty
{
	__DeclareClass(CPropPlrCharacterInput);
	__DeclarePropertyStorage;

protected:

	bool Enabled;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	virtual void	ActivateInput();
    virtual void	DeactivateInput();
	void			EnableInput(bool Enable);

	bool			OnMouseClick(Input::EMouseButton Button, bool Double = false);

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseDoubleClick, OnMouseDoubleClick);
	//DECLARE_EVENT_HANDLER(EnableInput, OnEnableInput);
	//DECLARE_EVENT_HANDLER(DisableInput, OnDisableInput);

public:

	CPropPlrCharacterInput(): Enabled(true) {}
};

}

#endif