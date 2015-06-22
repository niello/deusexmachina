#include <System/System.h> // keep before to disable warning
#include "DEMLogger.h"

namespace CEGUI
{

void CDEMLogger::logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level)
{
	if (Enable && level <= getLoggingLevel())
		switch (level) 
		{
			case CEGUI::Errors:
				//Sys::Error("%s\n", message.c_str());
				//break;
			case CEGUI::Standard:
			case CEGUI::Informative:
			case CEGUI::Insane:
				Sys::Log("%s\n", message.c_str());
				break;
			default:
				Sys::Error("Unknown CEGUI logging level\n");
        }
}
//---------------------------------------------------------------------

}