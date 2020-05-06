#pragma once
#ifndef __IPG_NEVENT_OBJ_DAMAGE_DONE_H__
#define __IPG_NEVENT_OBJ_DAMAGE_DONE_H__

#include <Events/EventNative.h>
#include <Combat/Dmg/DamageEffect.h>

// This msg is sent when the object receives damage

namespace Event
{

class ObjDamageDone: public Events::CEventNative
{
	NATIVE_EVENT_DECL;

public:

	int				Amount;
	Dmg::EDmgType	Type;
	CStrID			EntDamager;
};

}

#endif
