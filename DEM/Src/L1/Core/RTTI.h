#pragma once
#ifndef __DEM_L1_RTTI_H__
#define __DEM_L1_RTTI_H__

#include <kernel/ntypes.h>
#include <util/nstring.h>

// Implements the runtime type information system of Mangalore. Every class
// derived from CRefCounted should define a static RTTI object which is initialized
// with a pointer to a static string containing the human readable Name
// of the class, and a pointer to the static RTTI object of the Parent class.

namespace Core
{

class CRTTI
{
private:

	const nString Name;
	const CRTTI* Parent;

public:

	CRTTI(const nString& ClassName, const CRTTI* ParentClass);

	const nString& GetName() const { return Name; }
	const CRTTI* GetParent() const { return Parent; }

	bool IsDerivedFrom(const CRTTI& Other) const;

	bool operator ==(const CRTTI& rhs) const { return (this == &rhs); }
	bool operator !=(const CRTTI& rhs) const { return (this != &rhs); }
	//bool operator >(const CRTTI& rhs) const { return (this > &rhs); }
	//bool operator <(const CRTTI& rhs) const { return (this < &rhs); }
};
//---------------------------------------------------------------------

inline CRTTI::CRTTI(const nString& ClassName, const CRTTI* ParentClass):
	Name(ClassName),
	Parent(ParentClass)
{
	n_assert(ClassName != "");
	n_assert(ParentClass != this);
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(const CRTTI& Other) const
{
	const CRTTI* pCurr = this;
	while (pCurr)
	{
		if (pCurr == &Other) return true;
		pCurr = pCurr->Parent;
	}
	return false;
}
//---------------------------------------------------------------------

}

// Type declaration (header file).
#define DeclareRTTI \
public: \
    static Core::CRTTI RTTI; \
    virtual Core::CRTTI* GetRTTI() const;

// Type implementation (source file).
#define ImplementRTTI(type, ancestor) \
Core::CRTTI type::RTTI(#type, &ancestor::RTTI); \
Core::CRTTI* type::GetRTTI() const { return &this->RTTI; }

// Type implementation of topmost type in inheritance hierarchy (source file).
#define ImplementRootRtti(type) \
Core::CRTTI type::RTTI(#type, NULL); \
Core::CRTTI* type::GetRTTI() const { return &this->RTTI; }

#endif
