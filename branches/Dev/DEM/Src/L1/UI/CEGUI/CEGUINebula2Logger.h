#pragma once
#ifndef __DEM_L1_CEGUI_LOGGER_H__
#define __DEM_L1_CEGUI_LOGGER_H__

#include <CEGUILogger.h>

namespace CEGUI
{

class CNebula2Logger: public CEGUI::Logger
{
private:

	bool Enable;

public:

	CNebula2Logger(): Enable(true) {}

	virtual void	logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level);
	virtual void	setLogFilename(const CEGUI::String& filename, bool append) {}
	void			setEnable(bool isEnable) { Enable = isEnable; }
};

}

#endif
