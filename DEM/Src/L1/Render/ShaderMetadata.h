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

	//???implement some type safety?
	static void*				GetHandleData(HHandle Handle) { return HandleMgr.GetHandleData(Handle); }

	virtual EGPUFeatureLevel	GetMinFeatureLevel() const = 0;
	virtual HConst				GetConstHandle(CStrID ID) const = 0;
	virtual HConstBuffer		GetConstBufferHandle(HConst hConst) const = 0;
	virtual HResource			GetResourceHandle(CStrID ID) const = 0;
	virtual HSampler			GetSamplerHandle(CStrID ID) const = 0;
	virtual PShaderConstant		GetConstant(HConst hConst) const = 0;
};

}

#endif
