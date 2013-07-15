#pragma once
#ifndef __DEM_L1_OGG_FILE_H__
#define __DEM_L1_OGG_FILE_H__

#include "AudioFile.h"
#include <vorbis/codec.h>

namespace IO
{
	class CStream;
}

namespace Audio
{

class COGGFile: public CAudioFile
{
private:

	static const int    INPUTBUFFER = 4096;

	bool                m_bFileEndReached;
	int                 size;               /* ogg filesize in bytes */

	IO::CStream*		pStream;
	WAVEFORMATEX        wfx;                /* audio format */

	int                 bytesIn;
	char*               bufferIn;

	ogg_sync_state      oSyncState;         /* sync and verify incoming physical bitstream */
	ogg_stream_state    oStreamState;       /* take physical pages, weld into a logical stream of packets */
	ogg_page            oBitStreamPage;     /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet          oRawPacket;         /* one raw packet of data for decode */

	vorbis_info         vBitStreamInfo;     /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment      vComments;          /* struct that stores all the bitstream user comments */
	vorbis_dsp_state    vDecoderState;      /* central working state for the packet->PCM decoder */
	vorbis_block        vDecoderWorkSpace;  /* local working space for packet->PCM decode */

	bool                readEarlyLoopExit;  /* read:important to continue read-loop correctly */
	bool                endOfStream;        /* read:remember end of the stream */
	int                 keepAlive;

	bool                InitOGG();
	bool                ReleaseOGG();

public:

	COGGFile(): m_bFileEndReached(false), pStream(NULL), size(0) {}
	virtual ~COGGFile() { Close(); }

	virtual bool			Open(const CString& FileName);
	virtual void			Close();

	virtual uint			Read(void* pBuffer, uint BytesToRead);
	virtual bool			Reset();
	virtual int				GetSize() const { n_assert(pStream); return 0; }
	virtual WAVEFORMATEX*	GetFormat() { n_assert(pStream); return &wfx; }
};

}

#endif

