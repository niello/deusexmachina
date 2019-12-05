#pragma once
#ifndef __DEM_L1_RENDER_STATE_H__
#define __DEM_L1_RENDER_STATE_H__

#include <Core/Object.h>

// This class represents a state of the render device, or pipeline configuration,
// including programmable stages configured by shaders and possibly some settings,
// and fixed-function stages configured by settings.
// Internal representation of this object may differ significantly in different
// APIs, because they support different state settings. Independently of an exact
// state set, this class is designed for reuse of internal rendering API structures
// and for sorting by state to reduce potentially expensive state switches.
// State may be defined only partially, but some APIs like DX10+ group state changes.
// For that case, parent state must be provided at the creation time, and undefined
// values will be inherited from it. It allows to define a state hierarchy with
// partial overriding. Actual API objects will be created only when all required data
// is gathered from an hierarchy.

//!!!D3D12 encapsulates an input layout, primitive type, RT, DS formats inc MSAA here
//all except input layout are just restrictions, anyway OMSetRenderTargets, IASetPrimitiveTopology exist
//since input layout is baked inside, there will be one PSO per vertex buffer layout, if can't reuse

//!!!can control wireframe here, for D3D11 can store second rasterizer state block!
//or device should store render states by groups for device settings and choose at runtime
//second RS block is less wasteful!

//???!!!
//can store global sorting order here, set by device on a single off-line sorting, read by renderers on a fast state sorting
//device can sort with a recursively mirrored pattern, for example, and fast sorting compares just one value (and pointer comparison beforehand)

namespace Render
{

class CRenderState: public Core::CObject
{
	//RTTI_CLASS_DECL;

public:

	// manager must have a routine CParams desc -> CStrID key

	// need unique key (to reuse already created, or store already created as chunks, if so, no need in a totally unique key)
	// need comparison function or operator, which compares by a key and by values not included to the key, for creation
	// need comparison by internal representation, for 2 already created state objects
	// need sorting by the key and non-keyed values like a depth bias, to minimize state changes
};

typedef Ptr<CRenderState> PRenderState;

};

#endif