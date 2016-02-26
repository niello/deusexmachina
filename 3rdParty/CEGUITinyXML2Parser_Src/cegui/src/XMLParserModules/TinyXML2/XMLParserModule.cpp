#include "CEGUI/XMLParserModules/TinyXML2/XMLParserModule.h"
#include "CEGUI/XMLParserModules/TinyXML2/XMLParser.h"

CEGUI::XMLParser* createParser(void)
{
    return CEGUI_NEW_AO CEGUI::TinyXML2Parser();
}

void destroyParser(CEGUI::XMLParser* parser)
{
    CEGUI_DELETE_AO parser;
}
