#pragma once
#ifndef __DEM_L1_CEGUI_VIEWPORT_TARGET_H__
#define __DEM_L1_CEGUI_VIEWPORT_TARGET_H__

#include <UI/CEGUI/DEMRenderTarget.h>
#include <CEGUI/RenderTarget.h>

namespace CEGUI
{

class CDEMViewportTarget: public CDEMRenderTarget<RenderTarget>
{
public:

	CDEMViewportTarget(CDEMRenderer& owner, const Rectf& area);

	virtual bool isImageryCache() const { return false; }
};

}

#endif