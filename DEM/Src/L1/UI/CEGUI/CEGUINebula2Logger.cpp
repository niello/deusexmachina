#include <Core/Core.h> // keep before to disable warning
#include "CEGUINebula2Logger.h"

namespace CEGUI
{

void CNebula2Logger::logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level)
{
	if (Enable && level <= getLoggingLevel())
		switch (level) 
		{
			case CEGUI::Errors:
				//Core::Error("%s\n", message.c_str());
				//break;
			case CEGUI::Standard:
			case CEGUI::Informative:
			case CEGUI::Insane:
				Core::Log("%s\n", message.c_str());
				break;
			default:
				Core::Error("Unknown CEGUI logging level\n");
        }
}
//---------------------------------------------------------------------

} //namespace CEGUI
