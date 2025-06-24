#pragma once
#include <Data/Ptr.h>

// Plays a CBehaviourTreeAsset and tracks its state. Parallel tasks can be implemented using their own nested players.

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::AI
{
using PBehaviourTreeAsset = Ptr<class CBehaviourTreeAsset>;
enum class EBTStatus : U8;

class CBehaviourTreePlayer final
{
public:

	struct CDataStackRecord
	{
		std::byte* pNodeData;
		std::byte* pPrevStackTop;
	};

private:

	PBehaviourTreeAsset          _Asset;
	std::unique_ptr<std::byte[]> _MemBuffer;
	U16*                         _pNewStack = nullptr;           // Current traversal path
	U16*                         _pRequestStack = nullptr;       // Currently activated path overridden by requests from higher priority nodes
	U16*                         _pActiveStack = nullptr;        // Currently activated path
	CDataStackRecord*            _pNodeInstanceData = nullptr;   // Pointers to node instance data of active path nodes
	std::byte*                   _pInstanceDataBuffer = nullptr; // Top of the stack allocation buffer for node instance data
	U16                          _ActiveDepth = 0;               // Depth of _pActiveStack

	EBTStatus ActivateNode(U16 Index, CDataStackRecord& InstanceDataRecord);
	void      DeactivateNode(U16 Index, CDataStackRecord& InstanceDataRecord);

public:

	~CBehaviourTreePlayer();

	bool      Start(PBehaviourTreeAsset Asset);
	void      Stop();
	EBTStatus Update(Game::CGameSession& Session, float dt);

	CBehaviourTreeAsset* GetAsset() const { return _Asset.Get(); }
	bool                 IsPlaying() const { return _ActiveDepth > 0; } //???or !!_Asset?
};

}
