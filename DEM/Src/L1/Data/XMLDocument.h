#pragma once
#ifndef __DEM_L1_XML_DOCUMENT_H__
#define __DEM_L1_XML_DOCUMENT_H__

#include <Core/Object.h>
#include <TinyXML2/Src/tinyxml2.h>

// TinyXML-2 document wrapper with refcounting

namespace Data
{

class CXMLDocument: public tinyxml2::XMLDocument, public Core::CObject
{
public:
};

typedef Ptr<CXMLDocument> PXMLDocument;

}

//DECLARE_TYPE(PXMLDocument, 13)
//#define TXMLDocument	DATA_TYPE(PXMLDocument)

#endif
