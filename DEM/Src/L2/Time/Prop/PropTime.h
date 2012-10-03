#ifndef PROPERTIES_TIMEPROPERTY_H
#define PROPERTIES_TIMEPROPERTY_H
//------------------------------------------------------------------------------
/**
    @class Properties::TimeProperty

    The time property adds the attribute "Time" to the entity. This
    attributes contains the time since the time property has been
    attached to the entity.

    (C) 2005 Radon Labs GmbH
*/
#include "game/property.h"

namespace Properties
{

class TimeProperty: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(TimeProperty);

private:

	nTime attachTime;

public:

	TimeProperty(): attachTime(0.0) {}
	virtual ~TimeProperty() {}

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void Activate();
	virtual void OnBeginFrame();
};

RegisterFactory(TimeProperty);

}

#endif
