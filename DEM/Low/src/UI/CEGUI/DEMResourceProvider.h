#pragma once
#include "CEGUI/Base.h"
#include "CEGUI/ResourceProvider.h"
#include <map>

namespace CEGUI
{

class CDEMResourceProvider: public ResourceProvider
{
protected:

	std::map<String, String> ResourceGroups;

public:

	void           setResourceGroupDirectory(const String& resourceGroup, const String& directory);
	const String&  getResourceGroupDirectory(const String& resourceGroup);
	void           clearResourceGroupDirectory(const String& resourceGroup);

	virtual void   loadRawDataContainer(const String& filename, RawDataContainer& output, const String& resourceGroup) override;
	virtual void   unloadRawDataContainer(RawDataContainer& data) override;
	virtual size_t getResourceGroupFileNames(std::vector<String>& out_vec, const String& file_pattern, const String& resource_group) override;
};

}
