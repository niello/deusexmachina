#include "Faction.h"

namespace RPG
{

bool CFaction::AdoptMember(CStrID ID, int Rank)
{
	if (Members.Contains(ID)) FAIL;
	Members.Add(ID, Rank);
	OK;
}
//---------------------------------------------------------------------

CStrID CFaction::GetLeader() const
{
	//???use negative ranks?

	CStrID Leader;
	int MaxRank = 0;
	for (int i = 0; i < Members.GetCount(); ++i)
	{
		int Rank = Members.ValueAt(i);
		if (Rank > MaxRank)
		{
			MaxRank = Rank;
			Leader = Members.KeyAt(i);
		}
	}

	return Leader;
}
//---------------------------------------------------------------------

CStrID CFaction::GetGroupLeader(const CArray<CStrID>& Group) const
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