#pragma once
#ifndef __DEM_L1_IO_H__
#define __DEM_L1_IO_H__

#include <Data/Ptr.h>

// IO forward declarations

namespace IO
{
typedef Ptr<class IStream> PStream;

enum EStreamAccessMode
{
	SAM_READ		= 0x01,
	SAM_WRITE		= 0x02,
	SAM_APPEND		= 0x04,
	SAM_READWRITE	= SAM_READ | SAM_WRITE
};

enum EStreamAccessPattern
{
	SAP_DEFAULT,	// Stream must set it's own preference internally if this flag is provided in Open
	SAP_RANDOM,
	SAP_SEQUENTIAL
};

enum ESeekOrigin
{
	Seek_Begin,
	Seek_Current,
	Seek_End
};

enum EFSEntryType
{
	FSE_FILE	= 0x01,
	FSE_DIR		= 0x02,
	FSE_NONE	= 0x04	// No entry at all or invalid entry
};

}

#endif
