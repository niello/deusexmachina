#include <System/System.h> // keep before to disable warning
#include "DEMLogger.h"
#include <CEGUI/String.h>

namespace CEGUI
{

void CDEMLogger::logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level)
{
	if (Enable && level <= getLoggingLevel())
	{
#if CEGUI_STRING_CLASS == CEGUI_STRING_CLASS_UTF_32
		const std::string msg = String::convertUtf32ToUtf8(message.getString());
#else
		const std::string& msg = message.getString();
#endif

		switch (level)
		{
			case CEGUI::LoggingLevel::Error:
				Sys::Error("%s\n", msg.c_str());
				break;
			case CEGUI::LoggingLevel::Warning:
			case CEGUI::LoggingLevel::Standard:
			case CEGUI::LoggingLevel::Informative:
			case CEGUI::LoggingLevel::Insane:
				Sys::Log("%s\n", msg.c_str());
				break;
			default:
				Sys::Error("Unknown CEGUI logging level\n");
		}
	}
}
//---------------------------------------------------------------------

}
