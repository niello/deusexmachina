#include "GameStateManager.h"
#include <Game/GameState.h>

namespace DEM::Game
{

CGameStateManager::CGameStateManager() = default;
//---------------------------------------------------------------------

CGameStateManager::~CGameStateManager() = default;
//---------------------------------------------------------------------

void CGameStateManager::Update(double FrameTime)
{
	if (!_Stack.empty()) _Stack.back()->Update(FrameTime);
}
//---------------------------------------------------------------------

void CGameStateManager::PushState(PGameState NewState)
{
	if (!NewState) return;

	auto pCurrState = _Stack.empty() ? nullptr : _Stack.back().Get();
	if (pCurrState) pCurrState->OnNestedStatePushed(NewState); //???need?
	_Stack.push_back(NewState);
	_Stack.back()->OnEnter(pCurrState);
}
//---------------------------------------------------------------------

PGameState CGameStateManager::PopState()
{
	if (_Stack.empty()) return nullptr;

	PGameState CurrState = std::move(_Stack.back());
	_Stack.pop_back();
	auto pPrevState = _Stack.empty() ? nullptr : _Stack.back().Get();
	if (pPrevState) pPrevState->OnNestedStatePopping(CurrState); //???need?
	CurrState->OnExit(pPrevState);

	return std::move(CurrState);
}
//---------------------------------------------------------------------

void CGameStateManager::PopStateTo(const ::Core::CRTTI& StateType)
{
	while (!_Stack.empty() && !_Stack.back()->IsA(StateType))
		PopState();

	n_assert2(!_Stack.empty(), "CGameStateManager::PopStateTo() > requested state type not found, stack is empty");
}
//---------------------------------------------------------------------

void CGameStateManager::PopStateTo(const CGameState& State)
{
	while (!_Stack.empty() && _Stack.back() != &State)
		PopState();

	n_assert2(!_Stack.empty(), "CGameStateManager::PopStateTo() > requested state instance not found, stack is empty");
}
//---------------------------------------------------------------------

void CGameStateManager::PopAllStates()
{
	while (!_Stack.empty())
		PopState();
}
//---------------------------------------------------------------------

CGameState* CGameStateManager::FindState(const ::Core::CRTTI& StateType)
{
	for (auto It = _Stack.rbegin(); It != _Stack.rend(); ++It)
		if ((*It)->IsA(StateType)) return It->Get();

	return nullptr;
}
//---------------------------------------------------------------------


}
