#pragma once
#ifndef __DEM_L3_CHARACTER_SHEET_H__
#define __DEM_L3_CHARACTER_SHEET_H__

#include <Core/Object.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>

// Character sheet represents a character in terms of a role system. It contains all the
// stats, parameters, modifiers, states, effects etc.
// This sheet is based on the Potential Role System.

namespace RPG
{

// Primary characteristics
enum EStat
{
	Stat_Strength,
	Stat_Endurance,
	Stat_Perception,
	Stat_Dexterity,
	Stat_Erudition,
	Stat_Learning,
	Stat_Charisma,
	Stat_WillPower,

	Stat_Count
};

enum EPose
{
	Pose_Stand,
	Pose_Crouch,
	Pose_Sit,
	Pose_Lay,
	Pose_Falling	//???here or in movements?
};

enum EMentalState
{
	Mental_Normal,
	Mental_Careless,
	Mental_Intent,
	Mental_Panic,
	Mental_Fear,
	Mental_Horrified,
	Mental_Insane,
	Mental_Unconscious,
	Mental_Dead
};

class CCharacterSheet
{
protected:

	short			StatBase[Stat_Count];
	short			StatCurr[Stat_Count];

	short			HPBorn;

	EPose			Pose;
	EMentalState	MentalState;

public:

	short	GetStatBase(EStat Stat) const { return StatBase[Stat]; }
	short	GetStatCurr(EStat Stat) const { return StatCurr[Stat]; }
	short	GetStatModifier(EStat Stat) const { return StatCurr[Stat] - 11; }

	short	GetHPMax() const { HPBorn + StatCurr[Stat_Endurance] - 1; }
};

}

#endif
