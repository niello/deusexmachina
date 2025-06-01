#pragma once
#include <Data/FourCC.h>
#include <string>

// Implements the runtime type information system of Mangalore. Every class derived
// from CRTTIBaseClass should define a static RTTI object which is initialized
// with a pointer to a static string containing the human readable Name
// of the class, and a pointer to the static RTTI object of the Parent class.

namespace DEM::Core
{
class CRTTIBaseClass;

class CRTTI final
{
private:

	typedef CRTTIBaseClass* (*CFactoryFunc)();
	typedef CRTTIBaseClass* (*CFactoryInplaceFunc)(void* pMemAddr);

	const CRTTI*        pParent = nullptr;

	CFactoryFunc        pFactoryFunc;
	CFactoryInplaceFunc pInplaceFactoryFunc;

	std::string         Name;
	UPTR                InstanceSize;
	UPTR                InstanceAlignment;
	Data::CFourCC       FourCC;

public:

	// TODO: constexpr?
	CRTTI(const char* pClassName, Data::CFourCC ClassFourCC, CFactoryFunc pCreate, CFactoryInplaceFunc pCreateInplace, const CRTTI* pParentClass, UPTR InstSize, UPTR Alignment)
		: Name(pClassName)
		, FourCC(ClassFourCC)
		, InstanceSize(InstSize)
		, InstanceAlignment(Alignment)
		, pParent(pParentClass)
		, pFactoryFunc(pCreate)
		, pInplaceFactoryFunc(pCreateInplace)
	{
		n_assert(pClassName && *pClassName);
		n_assert(!IsBaseOf(pParentClass));
	}

	CRTTIBaseClass*    CreateInstance() const { return pFactoryFunc ? pFactoryFunc() : nullptr; }
	CRTTIBaseClass*    CreateInstance(void* pMemAddr) const { return pInplaceFactoryFunc ? pInplaceFactoryFunc(pMemAddr) : nullptr; }
	//void*            AllocInstanceMemory() const { return std::malloc(InstanceSize); }
	//void             FreeInstanceMemory(void* pPtr) { std::free(pPtr); }

	const std::string& GetName() const { return Name; }
	Data::CFourCC      GetFourCC() const { return FourCC; }
	const CRTTI*       GetParent() const { return pParent; }
	UPTR               GetInstanceSize() const { return InstanceSize; }
	UPTR               GetInstanceAlignment() const { return InstanceAlignment; }
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
	inline static const DEM::Core::CRTTI RTTI = DEM::Core::CRTTI(#Class, 0, nullptr, nullptr, &ParentClass::RTTI, 0, 0); \
	virtual const DEM::Core::CRTTI* GetRTTI() const override { return &RTTI; } \
private:

//	void* operator new(size_t size) { return RTTI.AllocInstanceMemory(); };
//	void operator delete(void* p) { RTTI.FreeInstanceMemory(p); };
#define FACTORY_CLASS_DECL \
private: \
	struct RegisterInFactory { RegisterInFactory(); }; \
	static RegisterInFactory FactoryHelper; \
public: \
	static const DEM::Core::CRTTI     RTTI; \
	virtual const DEM::Core::CRTTI*   GetRTTI() const override { return &RTTI; } \
	static DEM::Core::CRTTIBaseClass* CreateInstance(); \
	static DEM::Core::CRTTIBaseClass* CreateInstance(void* pMemAddr); \
	static void                       ForceFactoryRegistration(); \
private:

#define FACTORY_CLASS_IMPL(Class, FourCC, ParentClass) \
	const DEM::Core::CRTTI Class::RTTI = DEM::Core::CRTTI(#Class, FourCC, Class::CreateInstance, Class::CreateInstance, &ParentClass::RTTI, sizeof(Class), alignof(Class)); \
	DEM::Core::CRTTIBaseClass* Class::CreateInstance() { return new Class(); } \
	DEM::Core::CRTTIBaseClass* Class::CreateInstance(void* pMemAddr) { return new(pMemAddr) Class(); } \
	Class::RegisterInFactory::RegisterInFactory() { DEM::Core::CFactory::Instance().Register(Class::RTTI, #Class, FourCC); } \
	Class::RegisterInFactory Class::FactoryHelper{}; \
	void Class::ForceFactoryRegistration() { Class::FactoryHelper; }
