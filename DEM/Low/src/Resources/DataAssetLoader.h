#pragma once
#include <Resources/ResourceLoader.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/SerializeToParams.h>
#include <Data/FunctionTraits.h>

// A template for loading arbitrary C++ object as a DEM resource from HRD using metadata defined with DEM::Meta.
// Example of usage:
//
// class CAsset : public Core::CObject
// {
// public:
//
//     std::map<CStrID, size_t> Field1;
//     size_t                   Field2;
// };
//
// namespace DEM::Meta
// {
// template<> inline constexpr auto RegisterClassName<CAsset>() { return "CAsset"; }
// template<> inline constexpr auto RegisterMembers<CAsset>() { return std::make_tuple(DEM_META_MEMBER_FIELD(CAsset, 1, Field1), DEM_META_MEMBER_FIELD(CAsset, 2, Field2)); }
// }
//
// App.ResourceManager().RegisterDefaultCreator("hrd", &CAsset::RTTI, n_new(Resources::CDataAssetLoaderHRD<CAsset>(App.ResourceManager())));

namespace Resources
{
HAS_METHOD_WITH_SIGNATURE_TRAIT(OnPostLoad);

template<typename T>
class CDataAssetLoaderHRD : public Resources::CResourceLoader
{
public:

	static_assert(std::is_base_of_v<Core::CObject, T>, "Resource class must be derived from Core::CObject");

	CDataAssetLoaderHRD(Resources::CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override { return T::RTTI; }

	virtual Core::PObject CreateResource(CStrID UID) override
	{
		// Load data stream contents to a buffer
		const char* pOutSubId;
		Data::PBuffer Buffer;
		{
			IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
			if (!Stream || !Stream->IsOpened()) return nullptr;
			Buffer = Stream->ReadAll();
		}
		if (!Buffer) return nullptr;

		// Read params from resource HRD
		// FIXME: add an ability to create CData from CParams instead of PParams?
		//Data::CParams Params;
		Data::PParams Params(n_new(Data::CParams)());
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), *Params)) return nullptr;

		// Deserialize HRD to the C++ object using metadata
		Ptr<T> NewObject(n_new(T()));
		DEM::ParamsFormat::Deserialize(Data::CData(Params), *NewObject);

		// Do optional postprocessing
		if constexpr (has_method_with_signature_OnPostLoad_v<T, void()>)
			NewObject->OnPostLoad();

		return NewObject;
	}
};

}
