#pragma once
#ifndef __DEM_L1_DB_COLUMN_H__
#define __DEM_L1_DB_COLUMN_H__

#include "AttrID.h"

// Describes a column in a database table. Mainly a wrapper around attribute id,
// but contains additional data, like if the column is indexed, or if it is a primary key.

namespace DB
{

class CColumn
{
public:

	enum EType
	{
		Default, 
		Primary, 
		Indexed
	};

	CAttrID	AttrID;
	EType	Type;
	bool	Committed;

	CColumn(): Type(Default), Committed(false) {}
	CColumn(CAttrID ID, EType _Type = Default): AttrID(ID), Type(_Type), Committed(false) {}

	CStrID			GetName() const { return AttrID->GetName(); }
	const CType*	GetValueType() const { return AttrID->GetType(); }
	AccessMode		GetAccessMode() const { return AttrID->GetAccessMode();; }
};

}

#endif