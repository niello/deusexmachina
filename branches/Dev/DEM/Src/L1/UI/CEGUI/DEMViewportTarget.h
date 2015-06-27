#pragma once
#ifndef __DEM_L1_CEGUI_VIEWPORT_TARGET_H__
#define __DEM_L1_CEGUI_VIEWPORT_TARGET_H__

#include "DEMRenderTarget.h"

namespace CEGUI
{

class CDEMViewportTarget: public CDEMRenderTarget<>
{
public:

	CDEMViewportTarget(CDEMRenderer& owner);
	CDEMViewportTarget(CDEMRenderer& owner, const Rectf& area): CDEMRenderTarget<>(owner) { setArea(area); }

	bool isImageryCache() const { return false; }
};

}

#endif