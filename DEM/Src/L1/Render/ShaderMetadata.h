#pragma once
#ifndef __DEM_L1_RENDER_SHADER_METADATA_H__
#define __DEM_L1_RENDER_SHADER_METADATA_H__

#include <Render/RenderFwd.h>
#include <Data/HandleManager.h>

// Shader metadata describes its uniform inputs, resource bindings and hardware reqiurements.

namespace Render
{

class IShaderMetadata
{
protected:

	// Implementations should register their metadata objects in this
	// handle manager to return constant, CB, resource and sampler handles.
	//???pointer + explicit dynamic allocation/deallocation + optional injection of external manager?
	static Data::CHandleManager HandleMgr;

	//???add protected thread-safe HHandle RegisterMetadataObject(void*) method?

public:

	virtual ~IShaderMetadata() {}

	static void*				GetHandleData(HHandle Handle);

	virtual EGPUFeatureLevel	GetMinFeatureLevel() const = 0;
	virtual HConst				GetConstHandle(CStrID ID) const = 0;
	virtual HConstBuffer		GetConstBufferHandle(CStrID ID) const = 0;
	virtual HResource			GetResourceHandle(CStrID ID) const = 0;
	virtual HSampler			GetSamplerHandle(CStrID ID) const = 0;
	virtual bool				GetConstDesc(CStrID ID, CShaderConstDesc& Out) const = 0;

	//???IShaderVariable* CreateVariable(HConst hConst)? cache offset, size etc inside a variable object
};

}

#endif
