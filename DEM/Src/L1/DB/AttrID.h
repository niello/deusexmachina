#pragma once
#ifndef __DEM_L1_DB_ATTR_ID_H__
#define __DEM_L1_DB_ATTR_ID_H__

#include <Data/Data.h>
#include <Data/StringID.h>
#include <util/HashTable.h>

namespace DB
{
using namespace Data;

//!!!to Attr:: namespace!
enum AccessMode
{
	ReadOnly,
	ReadWrite
};

class CAttributeID
{
protected:

	CStrID	Name;
	char	Flags;			// AccessMode, IsStorable if needed etc
	CData	DefaultValue;	// Data type inside (Empty data can be used to create dynamically per-row typed columns)

public:

	CAttributeID() {} //!!!Default constructor WAS protected!

	CAttributeID(CStrID _Name, /*FourCC, */ char _Flags, const CData& DefaultVal);

	CStrID				GetName() const { return Name; }
	const CType*		GetType() const { return DefaultValue.GetType(); }
	template<class T>
	bool				IsA() const { return DefaultValue.IsA<T>(); }
	bool				IsStorable() const;
	bool				IsWritable() const;
	AccessMode			GetAccessMode() const { return (AccessMode)Flags; }
	const CData&		GetDefaultValue() const { return DefaultValue; }
};
//---------------------------------------------------------------------

typedef const CAttributeID* CAttrID;

inline CAttributeID::CAttributeID(CStrID _Name, /**/ char _Flags, const CData& DefaultVal):
	Name(_Name), Flags(_Flags), DefaultValue(DefaultVal)
{
}
//---------------------------------------------------------------------

#define BEGIN_ATTRS_REGISTRATION(Module) namespace Attr { void RegisterAttrs_##Module() {
#define END_ATTRS_REGISTRATION } };

#define DeclareAttrsModule(Module) void RegisterAttrs_##Module();
#define RegisterAttrs(Module) Attr::RegisterAttrs_##Module();

#define DeclareAttr(NAME) extern DB::CAttrID NAME;
#define DefineAttr(NAME) DB::CAttrID NAME(NULL);
#define RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL) NAME = DBSrv->RegisterAttrID(#NAME, DB::ACCESSMODE, DEFVAL);

#define RegisterVarAttr(NAME,ACCESSMODE) RegisterAttrWithDefault(NAME,ACCESSMODE,Data::CData())

#define DeclareBool(NAME) DeclareAttr(NAME)
#define DefineBool(NAME) DefineAttr(NAME)
#define RegisterBool(NAME,ACCESSMODE) RegisterBoolWithDefault(NAME,ACCESSMODE,false)
#define RegisterBoolWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareInt(NAME) DeclareAttr(NAME)
#define DefineInt(NAME) DefineAttr(NAME)
#define RegisterInt(NAME,ACCESSMODE) RegisterIntWithDefault(NAME,ACCESSMODE,0)
#define RegisterIntWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareFloat(NAME) DeclareAttr(NAME)
#define DefineFloat(NAME) DefineAttr(NAME)
#define RegisterFloat(NAME,ACCESSMODE) RegisterFloatWithDefault(NAME,ACCESSMODE,0.0f)
#define RegisterFloatWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareString(NAME) DeclareAttr(NAME)
#define DefineString(NAME) DefineAttr(NAME)
#define RegisterString(NAME,ACCESSMODE) RegisterStringWithDefault(NAME,ACCESSMODE,(nString("")))
#define RegisterStringWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareStrID(NAME) DeclareAttr(NAME)
#define DefineStrID(NAME) DefineAttr(NAME)
#define RegisterStrID(NAME,ACCESSMODE) RegisterStrIDWithDefault(NAME,ACCESSMODE,(CStrID::Empty))
#define RegisterStrIDWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareFloat4(NAME) DeclareAttr(NAME)
#define DefineFloat4(NAME) DefineAttr(NAME)
#define RegisterFloat4(NAME,ACCESSMODE) RegisterFloat4WithDefault(NAME,ACCESSMODE,(vector4(0,0,0,0)))
#define RegisterFloat4WithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

//!!!tmp!
#define DeclareVector4(NAME) DeclareFloat4(NAME)
#define DefineVector4(NAME) DefineAttr(NAME)
#define RegisterVector4(NAME,ACCESSMODE) RegisterFloat4(NAME,ACCESSMODE)
#define RegisterVector4WithDefault(NAME,ACCESSMODE,DEFVAL) RegisterFloat4WithDefault(NAME,ACCESSMODE,DEFVAL)
#define DeclareVector3(NAME) DeclareFloat4(NAME)
#define DefineVector3(NAME) DefineAttr(NAME)
#define RegisterVector3(NAME,ACCESSMODE) RegisterFloat4(NAME,ACCESSMODE)
#define RegisterVector3WithDefault(NAME,ACCESSMODE,DEFVAL) RegisterFloat4WithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareMatrix44(NAME) DeclareAttr(NAME)
#define DefineMatrix44(NAME) DefineAttr(NAME)
#define RegisterMatrix44(NAME,ACCESSMODE) RegisterMatrix44WithDefault(NAME,ACCESSMODE,(matrix44()))
#define RegisterMatrix44WithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)

#define DeclareBlob(NAME) DeclareAttr(NAME)
#define DefineBlob(NAME) DefineAttr(NAME)
#define RegisterBlob(NAME,ACCESSMODE) RegisterBlobWithDefault(NAME,ACCESSMODE,(Data::CBuffer()))
#define RegisterBlobWithDefault(NAME,ACCESSMODE,DEFVAL) RegisterAttrWithDefault(NAME,ACCESSMODE,DEFVAL)
}

#endif
