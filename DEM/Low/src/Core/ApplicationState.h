#pragma once
#include <Core/Object.h>

// Application state is a base class for user application logic. Application can
// switch between different states as a regular FSM. State transition has no
// duration, so all timed transition logic must be implemented inside a state.
// This class is not intended to be stateless and can store any data.

namespace DEM
{

namespace Core
{
class CApplication;
typedef Ptr<class CApplicationState> PApplicationState;

class CApplicationState : public ::Core::CObject
{
	__DeclareClassNoFactory;

public:

	virtual void				OnEnter(CApplication& App, CApplicationState* pFromState) {}
	virtual void				OnExit(CApplication& App, CApplicationState* pToState) {}
	virtual CApplicationState*	Update(CApplication& App, double FrameTime) { return this; }
};

}
};
