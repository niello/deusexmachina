#include "Faction.h"

namespace RPG
{

CStrID CFaction::GetGroupLeader(const nArray<CStrID>& Group) const
{
	//???use negative ranks?

	CStrID GroupLeader;
	int MaxRank = 0;
	for (int i = 0; i < Group.GetCount(); ++i)
	{
		int Idx = Members.FindIndex(Group[i]);
		if (Idx != INVALID_INDEX)
		{
			int Rank = Members.ValueAt(Idx);
			if (Rank > MaxRank)
			{
				MaxRank = Rank;
				GroupLeader = Group[i];
			}
		}
	}

	return GroupLeader;
}
//---------------------------------------------------------------------

}