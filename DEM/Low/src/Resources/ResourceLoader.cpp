#pragma once
#include <Resources/ResourceLoader.h>
#include <IO/IOServer.h>
#include <IO/Stream.h>

namespace Resources
{

IO::PStream CResourceLoader::OpenStream(CStrID UID, const char*& pOutSubId)
{
	if (!pIO) return nullptr;

	IO::PStream Stream;

	pOutSubId = strchr(UID.CStr(), '#');
	if (pOutSubId)
	{
		if (pOutSubId == UID.CStr()) return nullptr;

		CString Path(UID.CStr(), pOutSubId - UID.CStr());
		Stream = pIO->CreateStream(Path);

		++pOutSubId; // Skip '#'
		if (*pOutSubId == 0) pOutSubId = nullptr;
	}
	else Stream = pIO->CreateStream(UID.CStr());

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead())
	{
		return nullptr;
	}

	return Stream;
}
//---------------------------------------------------------------------

}
