#pragma once
#ifndef __DEM_L1_XML_DOCUMENT_H__
#define __DEM_L1_XML_DOCUMENT_H__

#include <Data/RefCounted.h>

// TinyXML-2 document wrapper with refcounting

namespace Data
{

class CXMLDocument: /*public tinyxml2::XMLDocument,*/ public Data::CRefCounted
{
public:
};

typedef Ptr<CXMLDocument> PXMLDocument;

}

//???declare PObject instead?
//DECLARE_TYPE(PXMLDocument, 13)
//#define TXMLDocument	DATA_TYPE(PXMLDocument)

#endif
