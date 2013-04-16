#include <StdCfg.h>
#include "CEGUINebula2Logger.h"
#include <kernel/ntypes.h>

namespace CEGUI
{

void CNebula2Logger::logEvent(const CEGUI::String& message, CEGUI::LoggingLevel level)
{
	if (Enable && level <= getLoggingLevel())
		switch (level) 
		{
			case CEGUI::Errors:
				//n_error("%s\n", message.c_str());
				//break;
			case CEGUI::Standard:
			case CEGUI::Informative:
			case CEGUI::Insane:
				n_printf("%s\n", message.c_str());
				break;
			default:
				n_error("Unknown CEGUI logging level\n");
        }
}
//---------------------------------------------------------------------

} //namespace CEGUI
