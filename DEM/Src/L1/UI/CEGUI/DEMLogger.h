#pragma once
#ifndef __DEM_L1_CEGUI_LOGGER_H__
#define __DEM_L1_CEGUI_LOGGER_H__

#include <CEGUI/Logger.h>

namespace CEGUI
{

class CDEMLogger: public CEGUI::Logger
{
private:

	bool Enable;

public:

	CDEMLogger(): Enable(true) {}

	virtual void	logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level);
	virtual void	setLogFilename(const CEGUI::String& filename, bool append) {}
	void			setEnable(bool isEnable) { Enable = isEnable; }
};

}

#endif
