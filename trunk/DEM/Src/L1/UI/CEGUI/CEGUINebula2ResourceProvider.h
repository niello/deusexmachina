#pragma once
#ifndef __DEM_L1_CEGUI_N2_RESOURCE_PROVIDER_H__
#define __DEM_L1_CEGUI_N2_RESOURCE_PROVIDER_H__

#include "CEGUIBase.h"
#include "CEGUIResourceProvider.h"

#include <util/ndictionary.h>

namespace CEGUI
{

class CNebula2ResourceProvider: public ResourceProvider
{
protected:

	nDictionary<String, String>	ResourceGroups;

public:

	CNebula2ResourceProvider() {}
	~CNebula2ResourceProvider(void) {}

	void			setResourceGroupDirectory(const String& resourceGroup, const String& directory);
	const String&	getResourceGroupDirectory(const String& resourceGroup);

	void			clearResourceGroupDirectory(const String& resourceGroup);

	void			loadRawDataContainer(const String& filename, RawDataContainer& output, const String& resourceGroup);
	void			unloadRawDataContainer(RawDataContainer& data);
	size_t			getResourceGroupFileNames(std::vector<String>& out_vec, const String& file_pattern,
											  const String& resource_group);
};

}

#endif
