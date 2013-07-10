#pragma once
#ifndef __DEM_L3_FACTION_H__
#define __DEM_L3_FACTION_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Faction is a group of characters that are on the same side of a conflict or has a common interest or goal.
// Faction typically has a leader, and members can be ranked to determine weight or hierarchy of members.
// Party of player characters is also a faction.

namespace RPG
{

class CFaction: public Core::CRefCounted
{
protected:

	nDictionary<CStrID, int> Members; // Value is a rank, 0 is reserved for non-members

	//???relation/attitude info towards other factions?
	//???relation/attitude info towards characters?
	// if record is missed, Att_Unaware (0 or very negative number)
	//!!!can share attitude with character subsystem!

public:

	bool	AdoptMember(CStrID ID, int Rank); //???bool IsNativeMember or use sign of rank?
	bool	ExpelMember(CStrID ID);

	DWORD	SplitByMembership(const nArray<CStrID>& Group, nArray<CStrID>* pMembers = NULL, nArray<CStrID>* pNonMembers = NULL) const;

	DWORD	GetMemberCount() const { return Members.GetCount(); }
	//CStrID	GetMember(DWORD Idx) const;
	bool	IsMember(CStrID ID) const { return Members.Contains(ID); }
	int		GetMemberRank(CStrID ID) const;
	int		SetMemberRank(CStrID ID, int Rank) const; // Ret prev value
	//bool	IsNativeMember(CStrID ID) const { return GetMemberRank(ID) > 0; }
	CStrID	GetLeader() const;
	bool	IsLeader(CStrID ID) const { return GetLeader() == ID; }
	CStrID	GetGroupLeader(const nArray<CStrID>& Group) const;

	//???int GetAttitudeTo[wards](CStrID Character)
	//???int GetAttitudeTo[wards](CStrID OtherFaction)
	//???where will be GroupHasItem / FactionHasItem?
};

typedef Ptr<CFaction> PFaction;

}

#endif
