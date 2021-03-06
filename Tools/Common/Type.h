#pragma once

// Template data type implementation.

#define INVALID_TYPE_ID (-1)

#define DECLARE_TYPE(T, ID)					namespace Data { template<> class CTypeID<T> { public: enum { TypeID = ID }; enum { IsDeclared = true }; }; }
#define DEFINE_TYPE(T, DEFAULT)				static CTypeImpl<T> DataType_##T; const CType* CTypeImpl<T>::Type = &DataType_##T; const T CTypeImpl<T>::DefaultValue = DEFAULT;
#define DEFINE_TYPE_EX(T, Name, DEFAULT)	static CTypeImpl<T> DataType_##Name; const CType* CTypeImpl<T>::Type = &DataType_##Name; const T CTypeImpl<T>::DefaultValue = DEFAULT;
#define DATA_TYPE(T)						(Data::CType::GetType<T>())
#define DATA_TYPE_ID(T)						(Data::CType::GetTypeID<T>())
#define DATA_TYPE_NV(T)						Data::CTypeImpl<T>::GetNVType()->CTypeImpl<T>

namespace Data
{
//if const void* Value <=> const void** pSrcObj ambiguity, use forex size_t** instead of void**

class CType
{
protected:

	virtual ~CType() {}

public:

	virtual void		New(void** pObj) const = 0;
	virtual void		New(void** pObj, void* const* pSrcObj) const = 0;
	virtual void		NewT(void** pObj, const void* Value) const = 0;
	virtual void		Delete(void** pObj) const = 0;
	virtual void		Copy(void** pObj, void* const* pSrcObj) const = 0;
	virtual void		CopyT(void** pObj, const void* Value) const = 0;
	virtual bool		IsEqual(const void* pObj, const void* pOtherObj) const = 0;
	virtual bool		IsEqualT(const void* pObj, const void* OtherValue) const = 0;
	virtual int			GetSize() const = 0;
	virtual int			GetID() const = 0;
	virtual const char*	ToString(const void* pObj) const = 0;

	template<class T>
	static const CType*	GetType() { static_assert(CTypeID<T>::IsDeclared, "Type not declared!"); return CTypeImpl<T>::Type; }
	template<class T>
	static int			GetTypeID() { static_assert(CTypeID<T>::IsDeclared, "Type not declared!"); return CTypeID<T>::TypeID; }
};

// Needed only for assertions, can remove when disable asserts
template<class T>
class CTypeID
{
public:

	enum { TypeID = INVALID_TYPE_ID }; //???use fourcc?
	enum { IsDeclared = false };
	//can store debug type name here
};

template<class T>
class CTypeImpl: public CType
{
public:
	
	static const CType* Type;
	static const T		DefaultValue;

	CTypeImpl() { static_assert(CTypeID<T>::IsDeclared, "Type not declared!"); }

	// Getter for non-virtual type instance (convenience method)
	static const CTypeImpl<T>*	GetNVType() { static_assert(CTypeID<T>::IsDeclared, "Type not declared!"); return (const CTypeImpl<T>*)Type; }

	virtual void		New(void** pObj) const;
	virtual void		New(void** pObj, void* const* pSrcObj) const;
	virtual void		NewT(void** pObj, const void* Value) const;
	virtual void		Delete(void** pObj) const;
	virtual void		Copy(void** pObj, void* const* pSrcObj) const;
	virtual void		CopyT(void** pObj, const void* Value) const;
	virtual bool		IsEqual(const void* pObj, const void* pOtherObj) const;
	virtual bool		IsEqualT(const void* pObj, const void* OtherValue) const;
	virtual int			GetSize() const;
	virtual int			GetID() const { return CTypeID<T>::TypeID; }
	virtual const char*	ToString(const void* /*pObj*/) const { return nullptr; }

	inline void*		GetPtr(void* pObj) const;
	inline const void*	GetPtr(const void* pObj) const;

	//convert, compare, save, load
};

template<class T> inline void CTypeImpl<T>::New(void** pObj) const
{
	if (sizeof(T) <= sizeof(void*)) new(pObj) T(DefaultValue);
	else *(T**)pObj = new T(DefaultValue);
}
//---------------------------------------------------------------------

template<class T> inline void CTypeImpl<T>::New(void** pObj, void* const* pSrcObj) const
{
	if (sizeof(T) <= sizeof(void*)) new(pObj) T(*(const T*)GetPtr(pSrcObj));
	else *(T**)pObj = new T(*(const T*)GetPtr(pSrcObj));
}
//---------------------------------------------------------------------

template<class T> inline void CTypeImpl<T>::NewT(void** pObj, const void* Value) const
{
	if (sizeof(T) <= sizeof(void*)) new(pObj) T(*(const T*)Value);
	else *(T**)pObj = new T(*(const T*)Value);
}
//---------------------------------------------------------------------

template<class T> inline void CTypeImpl<T>::Delete(void** pObj) const
{
	if (sizeof(T) <= sizeof(void*)) ((T*)pObj)->~T();
	else
	{
		if (*(T**)pObj) delete *(T**)pObj;
	}
	*pObj = 0; // 0 or nullptr
}
//---------------------------------------------------------------------

template<class T> inline void CTypeImpl<T>::Copy(void** pObj, void* const* pSrcObj) const
{
	if (sizeof(T) <= sizeof(void*)) *(T*)pObj = *(T*)pSrcObj;
	else
	{
		if (*(T**)pObj) **(T**)pObj = **(T**)pSrcObj;
		else *(T**)pObj = new T(*(const T*)GetPtr(pSrcObj));
	}
}
//---------------------------------------------------------------------

template<class T> inline void CTypeImpl<T>::CopyT(void** pObj, const void* Value) const
{
	if (sizeof(T) <= sizeof(void*)) *(T*)pObj = *(const T*)Value;
	else
	{
		if (*(T**)pObj) **(T**)pObj = *(const T*)Value;
		else *(T**)pObj = new T(*(const T*)Value);
	}
}
//---------------------------------------------------------------------

template<class T> inline void* CTypeImpl<T>::GetPtr(void* pObj) const
{
	return (sizeof(T) <= sizeof(void*)) ? pObj : *(T**)pObj;
}
//---------------------------------------------------------------------

template<class T> inline const void* CTypeImpl<T>::GetPtr(const void* pObj) const
{
	return (sizeof(T) <= sizeof(void*)) ? pObj : *(T**)pObj;
}
//---------------------------------------------------------------------

template<class T> inline bool CTypeImpl<T>::IsEqual(const void* pObj, const void* pOtherObj) const
{
	return (*(T*)GetPtr(pObj)) == (*(T*)GetPtr(pOtherObj));
}
//---------------------------------------------------------------------

template<class T> inline bool CTypeImpl<T>::IsEqualT(const void* pObj, const void* OtherValue) const
{
	return (*(T*)GetPtr(pObj)) == (*(const T*)OtherValue);
}
//---------------------------------------------------------------------

template<class T> inline int CTypeImpl<T>::GetSize() const
{
	return (sizeof(T) <= sizeof(void*)) ? sizeof(void*) : sizeof(T);
}
//---------------------------------------------------------------------

};
