#ifndef _CEGUITinyXML2ParserModule_h_
#define _CEGUITinyXML2ParserModule_h_

#include "CEGUI/XMLParserModules/TinyXML2/XMLParser.h"

/*!
\brief
    exported function that creates an XMLParser based object and returns
    a pointer to that object.
*/
extern "C" CEGUITINYXML2PARSER_API CEGUI::XMLParser* createParser(void);

/*!
\brief
    exported function that deletes an XMLParser based object previously
    created by this module.
*/
extern "C" CEGUITINYXML2PARSER_API void destroyParser(CEGUI::XMLParser* parser);

#endif // end of guard _CEGUITinyXMLParserModule_h_
