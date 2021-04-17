#include "ScriptedAbility.h"

namespace DEM::Game
{

vector3 CScriptedAbility::GetInteractionPoint() const
{
	NOT_IMPLEMENTED;
	return {};
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetFacingParams(CFacingParams& Out) const
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnStart() const
{
}
//---------------------------------------------------------------------

EActionStatus CScriptedAbility::OnUpdate() const
{
	NOT_IMPLEMENTED;
	return EActionStatus::Active;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnEnd() const
{
}
//---------------------------------------------------------------------

}
