#include "PropActorAnimation.h"

#include <Game/Entity.h>
#include <Gfx/Event/GfxSetAnimation.h>

#include <Loading/EntityFactory.h>

namespace Properties
{
ImplementRTTI(Properties::CPropActorAnimation, Game::CProperty);
ImplementFactory(Properties::CPropActorAnimation);
ImplementPropertyStorage(CPropActorAnimation, 64);
RegisterProperty(CPropActorAnimation);

void CPropActorAnimation::Activate()
{
	Game::CProperty::Activate();

	RequestAnimation("Idle", "", n_rand(0.0f, 1.0f));

	PROP_SUBSCRIBE_PEVENT(ActorStopMoving, CPropActorAnimation, OnActorStopMoving);
	//PROP_SUBSCRIBE_PEVENT(ActorSetSpeed, CPropActorAnimation, OnMoveDirOrSpeed);
	//PROP_SUBSCRIBE_NEVENT(ActorMoveDir, CPropActorAnimation, OnMoveDirOrSpeed);
}
//---------------------------------------------------------------------

void CPropActorAnimation::Deactivate()
{
	UNSUBSCRIBE_EVENT(ActorStopMoving);
	//UNSUBSCRIBE_EVENT(ActorSetSpeed);
	//UNSUBSCRIBE_EVENT(ActorMoveDir);

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropActorAnimation::OnActorStopMoving(const CEventBase& Event)
{
	RequestAnimation("Idle", "", n_rand(0.0f, 1.0f));
	OK;
}
//------------------------------------------------------------------------------

/*
bool CPropActorAnimation::OnMoveDirOrSpeed(const CEventBase& Event)
{
	//RequestAnimation("Run", "", n_rand(0.0f, 1.0f));
	//RequestAnimation("Walk", "", n_rand(0.0f, 1.0f));
	OK;
}
//------------------------------------------------------------------------------
*/

void CPropActorAnimation::RequestAnimation(const nString& BaseAnimation,
										   const nString& OverlayAnimation,
										   float BaseAnimTimeOffset) const
{
	Event::GfxSetAnimation Event;
	Event.BaseAnim = BaseAnimation;
	Event.OverlayAnim = OverlayAnimation;
	Event.BaseAnimTimeOffset = BaseAnimTimeOffset;
	Event.FadeInTime = 0.2;
	GetEntity()->FireEvent(Event);
}
//---------------------------------------------------------------------

} // namespace Properties
