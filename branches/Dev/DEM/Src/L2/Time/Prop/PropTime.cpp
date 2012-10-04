#include "PropTime.h"

#include <Time/TimeServer.h>
#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineFloat(Time);
}

BEGIN_ATTRS_REGISTRATION(PropTime)
	RegisterFloat(Time, ReadWrite);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::TimeProperty, Game::CProperty);
ImplementFactory(Properties::TimeProperty);

using namespace Game;

void TimeProperty::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::Time);
}
//---------------------------------------------------------------------

void TimeProperty::Activate()
{
	CProperty::Activate();
	attachTime = GameSrv->GetTime();
}
//---------------------------------------------------------------------

void TimeProperty::OnBeginFrame()
{
	GetEntity()->Set<float>(Attr::Time, (float)(GameSrv->GetTime() - attachTime));
}
//---------------------------------------------------------------------

} // namespace Properties
