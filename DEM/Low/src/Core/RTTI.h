#pragma once
#include <Data/FourCC.h>
#include <string>

// Implements the runtime type information system of Mangalore. Every class derived
// from CRTTIBaseClass should define a static RTTI object which is initialized
// with a pointer to a static string containing the human readable Name
// of the class, and a pointer to the static RTTI object of the Parent class.

namespace Core
{
class CRTTIBaseClass;

class CRTTI final
{
private:

	typedef CRTTIBaseClass* (*CFactoryFunc)(void* pParam);

	const CRTTI* pParent;

	std::string   Name;
	Data::CFourCC FourCC;
	UPTR          InstanceSize;

	CFactoryFunc  pFactoryFunc;

public:


	// TODO: constexpr?
	CRTTI(const char* pClassName, Data::CFourCC ClassFourCC, CFactoryFunc pFactoryCreator, const CRTTI* pParentClass, UPTR InstSize)
		: Name(pClassName)
		, FourCC(ClassFourCC)
		, InstanceSize(InstSize)
		, pParent(pParentClass)
		, pFactoryFunc(pFactoryCreator)
	{
		n_assert(pClassName && *pClassName);
		n_assert(pParentClass != this);
	}

	CRTTIBaseClass*    CreateClassInstance(void* pParam = nullptr) const { return pFactoryFunc ? pFactoryFunc(pParam) : nullptr; }
	//void*            AllocInstanceMemory() const { return n_malloc(InstanceSize); }
	//void             FreeInstanceMemory(void* pPtr) { n_free(pPtr); }

	const std::string& GetName() const { return Name; }
	Data::CFourCC      GetFourCC() const { return FourCC; }
	const CRTTI*       GetParent() const { return pParent; }
	UPTR               GetInstanceSize() const { return InstanceSize; }
	bool               IsDerivedFrom(const CRTTI& Other) const;
	bool               IsDerivedFrom(Data::CFourCC OtherFourCC) const;
	bool               IsDerivedFrom(const char* pOtherName) const;

	bool IsBaseOf(const CRTTI* pRTTI) const
	{
		while (pRTTI)
		{
			if (pRTTI == this) return true;
			pRTTI = pRTTI->pParent;
		}

		return false;
	}

	bool operator ==(const CRTTI& Other) const { return this == &Other; }
	bool operator !=(const CRTTI& Other) const { return this != &Other; }
};

inline bool CRTTI::IsDerivedFrom(const CRTTI& Other) const
{
	const CRTTI* pCurr = this;
	do
	{
		if (pCurr == &Other) OK;
		pCurr = pCurr->pParent;
	}
	while (pCurr);
	FAIL;
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(Data::CFourCC OtherFourCC) const
{
	const CRTTI* pCurr = this;
	do
	{
		if (pCurr->FourCC == OtherFourCC) OK;
		pCurr = pCurr->pParent;
	}
	while (pCurr);
	FAIL;
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(const char* pOtherName) const
{
	const CRTTI* pCurr = this;
	do
	{
		if (pCurr->Name == pOtherName) OK;
		pCurr = pCurr->pParent;
	}
	while (pCurr);
	FAIL;
}
//---------------------------------------------------------------------

}

#define RTTI_CLASS_DECL(Class, ParentClass) \
public: \
	inline static const ::Core::CRTTI RTTI = ::Core::CRTTI(#Class, 0, nullptr, &ParentClass::RTTI, 0); \
	virtual const ::Core::CRTTI* GetRTTI() const override { return &RTTI; } \
private:

//	void* operator new(size_t size) { return RTTI.AllocInstanceMemory(); };
//	void operator delete(void* p) { RTTI.FreeInstanceMemory(p); };
#define FACTORY_CLASS_DECL \
private: \
	struct RegisterInFactory { RegisterInFactory(); }; \
	static RegisterInFactory FactoryHelper; \
public: \
	static const ::Core::CRTTI     RTTI; \
	virtual const ::Core::CRTTI*   GetRTTI() const override { return &RTTI; } \
	static ::Core::CRTTIBaseClass* CreateClassInstance(void* pParam = nullptr); \
	static void                    ForceFactoryRegistration(); \
private:

#define FACTORY_CLASS_IMPL(Class, FourCC, ParentClass) \
	const ::Core::CRTTI Class::RTTI = ::Core::CRTTI(#Class, FourCC, Class::CreateClassInstance, &ParentClass::RTTI, sizeof(Class)); \
	::Core::CRTTIBaseClass* Class::CreateClassInstance(void* pParam) { return n_new(Class); } \
	Class::RegisterInFactory::RegisterInFactory() { ::Core::CFactory::Instance().Register(Class::RTTI, #Class, FourCC); } \
	Class::RegisterInFactory Class::FactoryHelper{}; \
	void Class::ForceFactoryRegistration() { Class::FactoryHelper; }
