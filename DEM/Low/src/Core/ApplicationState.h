#pragma once
#include <Core/Object.h>

// Application state is a base class for user application logic. Application can
// switch between different states as a regular FSM. State transition has no
// duration, so all timed transition logic must be implemented inside a state.
// This class is not intended to be stateless and can store any data. Therefore
// it stores its host application because some stored objects may be app-specific.

namespace DEM::Core
{
class CApplication;
typedef Ptr<class CApplicationState> PApplicationState;

class CApplicationState : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Core::CApplicationState, DEM::Core::CObject);

protected:

	CApplication& App;

public:

	CApplicationState(CApplication& Application) : App(Application) {}

	virtual void				OnEnter(CApplicationState* pFromState) {}
	virtual void				OnExit(CApplicationState* pToState) {}
	virtual CApplicationState*	Update(double FrameTime) { return this; }

	CApplication&				GetApplication() const { return App; }
};

}
