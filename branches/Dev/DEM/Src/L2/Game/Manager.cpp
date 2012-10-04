#include "Manager.h"

namespace Game
{
ImplementRTTI(Game::CManager, Core::CRefCounted);

CManager::CManager(): _IsActive(false)
{
}
//---------------------------------------------------------------------

CManager::~CManager()
{
}
//---------------------------------------------------------------------

// This method is called when the manager is attached to the game server.
void CManager::Activate()
{
	n_assert(!_IsActive);
	_IsActive = true;
}
//---------------------------------------------------------------------

// This method is called when the manager is removed from the game server.
void CManager::Deactivate()
{
	n_assert(_IsActive);
	_IsActive = false;
}
//---------------------------------------------------------------------

} // namespace Game
