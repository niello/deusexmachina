#pragma once
#ifndef __DEM_L2_PROP_INPUT_H__
#define __DEM_L2_PROP_INPUT_H__

#include <Game/Property.h>
#include <Game/Entity.h>

/**
    An input property adds the ability to handle user input to an entity.
    If an CPropInput is attached to an entity it can become the input
    focus entity. Global input focus is managed by the Game::CFocusManager
    singleton.

    If you want the concept of an input focus in your application you should
    derive your own input property classes from the CPropInput class,
    because then the CFocusManager will be aware of it (otherwise it will
    just ignore the entity).

    Based on mangalore InputProperty_(C) 2005 Radon Labs GmbH
*/

namespace Properties
{

class CPropInput: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropInput);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

protected:

	bool Enabled;

	virtual void ActivateInput() {}
	virtual void DeactivateInput() {}

	DECLARE_EVENT_HANDLER(OnObtainInputFocus, OnObtainInputFocus);
	DECLARE_EVENT_HANDLER(OnLoseInputFocus, OnLoseInputFocus);
	DECLARE_EVENT_HANDLER(EnableInput, OnEnableInput);
	DECLARE_EVENT_HANDLER(DisableInput, OnDisableInput);

public:

	CPropInput(): Enabled(true) {}

    virtual void	Activate();
    virtual void	Deactivate();
	virtual bool	HasFocus() const;
	void			EnableInput(bool Enable);
};

RegisterFactory(CPropInput);

}

#endif
