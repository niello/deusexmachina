#include "WorldState.h"

namespace Data
{
DEFINE_TYPE_EX(AI::EWSProp, EWSProp, AI::EWSProp())
}

namespace AI
{

int CWorldState::GetDiffCount(const CWorldState& Other) const
{
	DWORD DiffCount = 0;

	for (int i = 0; i < WSP_Count; ++i)
	{
		bool IsSetSelf = Props[i].IsValid();
		bool IsSetOther = Other.Props[i].IsValid();

		if (IsSetSelf && IsSetOther)
		{
			if (Props[i] != Other.Props[i]) ++DiffCount;
		}
		else if (IsSetSelf || IsSetOther) ++DiffCount;
	}

	return DiffCount;
}
//---------------------------------------------------------------------

} //namespace AI