#include "CEGUI/XMLParserModules/TinyXML2/XMLParser.h"
#include "CEGUI/ResourceProvider.h"
#include "CEGUI/System.h"
#include "CEGUI/XMLHandler.h"
#include "CEGUI/XMLAttributes.h"
#include "CEGUI/Logger.h"
#include "CEGUI/Exceptions.h"
#include <tinyxml2.h>

// Start of CEGUI namespace section
namespace CEGUI
{
    TinyXML2Parser::TinyXML2Parser(void)
    {
        // set ID string
        d_identifierString = "CEGUI::TinyXML2Parser - TinyXML2 CEGUI XML parser implementation by Vladimir 'Niello' Orlov";
    }

    void TinyXML2Parser::parseXML(XMLHandler& handler, const RawDataContainer& source, const String& schemaName)
    {
        tinyxml2::XMLDocument doc;

        // Parse the document
		if (doc.Parse((const char*)source.getDataPtr(), source.getSize()) != tinyxml2::XML_SUCCESS)
		{
			CEGUI_THROW(FileIOException("TinyXML2Parser: an error occurred while "
                "parsing the XML document '...' - check it for potential errors!"));
        }

        const tinyxml2::XMLElement* currElement = doc.RootElement();
        if (currElement)
        {
            CEGUI_TRY
            {
                processElement(handler, currElement);
            }
            CEGUI_CATCH(...)
            {
                CEGUI_RETHROW;
            }
        }
    }

    void TinyXML2Parser::processElement(XMLHandler& handler, const tinyxml2::XMLElement* element)
    {
        // build attributes block for the element
        XMLAttributes attrs;

		const tinyxml2::XMLAttribute *currAttr = element->FirstAttribute();
        while (currAttr)
        {
            attrs.add((utf8*)currAttr->Name(), (utf8*)currAttr->Value());
            currAttr = currAttr->Next();
        }

        handler.elementStart((utf8*)element->Value(), attrs);

		const tinyxml2::XMLNode* pChildNode = element->FirstChild();
        while (pChildNode)
        {
			if (pChildNode->ToText())
			{
				const char* pValue = pChildNode->ToText()->Value();
                if (pValue && *pValue)
                    handler.text((utf8*)pValue);
			}
			else if (pChildNode->ToElement())
				processElement(handler, pChildNode->ToElement());
			
			// Silently ignore unhandled node type

			pChildNode = pChildNode->NextSibling();
        }

        handler.elementEnd((utf8*)element->Value());
    }

} // End of  CEGUI namespace section

