#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>
#include <Data/SerializeToParams.h>

// A container that wraps some actual resource object along with any information
// required for its management. This class is not intended to be a base class for
// specific resources. Inherit from a CObject instead.
// Resource UID may consist of 2 parts and must have at least one of them.
// Format is "engine:path/to/file.ext#Id". '#' is a delimiter, you can't
// use it as an identifier part. System processes UIDs as follows:
// a) "engine:path/to/file.ext" is loaded from file with 1 file - 1 resource relation;
// b) "#Id" is generated procedurally and is not associated with a file;
// c) "engine:path/to/file.ext#Id" is intended for loading from files that contain
//    multiple resources. Loader will use an "Id" part to select a subresource to load.

// TODO: can implement own refcounting and drop virtual table & destructor required by CRefCounted

namespace Resources
{
typedef Ptr<class IResourceCreator> PResourceCreator;
typedef Ptr<class CResource> PResource;

enum class EResourceState : U8
{
	NotLoaded,
	LoadingRequested,
	LoadingInProgress,
	Loaded,
	LoadingFailed,
	LoadingCancelled
};

class CResource final : public Data::CRefCounted
{
protected:

	CStrID             _UID;
	DEM::Core::PObject _Object;  // Actual object, such as a texture or a game object description
	PResourceCreator   _Creator; // For (re)creation of an actual resource object
	EResourceState     _State = EResourceState::NotLoaded;

public:

	CResource(CStrID UID);
	virtual ~CResource() override;

	template<class T> T* GetObject()
	{
		auto* pObj = GetObject();
		return (pObj && pObj->IsA<T>()) ? static_cast<T*>(pObj) : nullptr;
	}

	template<class T> T* ValidateObject()
	{
		auto* pObj = ValidateObject();
		return (pObj && pObj->IsA<T>()) ? static_cast<T*>(pObj) : nullptr;
	}

	DEM::Core::CObject* GetObject();
	DEM::Core::CObject* ValidateObject();
	void                Unload();

	//PJob ValidateObjectAsync(callback on finished); - doesn't create a job if already actual

	CStrID              GetUID() const { return _UID; }
	EResourceState      GetState() const { return _State; } //!!!must be thread-safe!
	bool                IsLoaded() const { return _State == EResourceState::Loaded; } //!!!must be thread-safe!
	IResourceCreator*   GetCreator() const { return _Creator.Get(); }
	void                SetCreator(IResourceCreator* pNewCreator);
};

}

namespace DEM::Serialization
{

template<>
struct ParamsFormat<Resources::PResource>
{
	static inline void Serialize(Data::CData& Output, Resources::PResource Value)
	{
		Output = Value ? Value->GetUID() : CStrID{};
	}

	static inline void Deserialize(const Data::CData& Input, Resources::PResource& Value)
	{
		if (auto pStrID = Input.As<CStrID>())
			Value = (*pStrID) ? n_new(Resources::CResource(*pStrID)) : nullptr;
		else if (auto pString = Input.As<CString>())
			Value = pString->IsValid() ? n_new(Resources::CResource(CStrID(pString->CStr()))) : nullptr;
		else
			Value = nullptr;
	}
};

}
