#ifndef _CEGUITinyXML2Parser_h_
#define _CEGUITinyXML2Parser_h_

#include "../../XMLParser.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUITINYXMLPARSER_EXPORTS
#       define CEGUITINYXML2PARSER_API __declspec(dllexport)
#   else
#       define CEGUITINYXML2PARSER_API __declspec(dllimport)
#   endif
#else
#   define CEGUITINYXML2PARSER_API
#endif

namespace tinyxml2
{
	class XMLElement;
}

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
    Implementation of XMLParser using TinyXML
     */
    class CEGUITINYXML2PARSER_API TinyXML2Parser : public XMLParser
    {
    public:
        TinyXML2Parser(void);
		~TinyXML2Parser(void) {}

        // Implementation of public abstract interface
        void parseXML(XMLHandler& handler, const RawDataContainer& filename, const String& schemaName);

    protected:
        // Implementation of abstract interface.
        bool initialiseImpl(void) { return true; }
        void cleanupImpl(void) {}
 
		static void processElement(XMLHandler& handler, const tinyxml2::XMLElement* element);
   };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUITinyXML2Parser_h_
