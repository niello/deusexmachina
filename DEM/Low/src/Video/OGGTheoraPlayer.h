#pragma once
#ifndef __DEM_L1_OGG_THEORA_PLAYER_H__
#define __DEM_L1_OGG_THEORA_PLAYER_H__

#include "VideoPlayer.h"
#include <Data/RefCounted.h>
#include <theora/theora.h>

// A videoplayer for *.ogg files
// Most parts are just copied out of the decoder-example of the theora-package
// and slightly modified to match the CVideoPlayer interface
// Based on nOggTheoraPlayer (C) 2005 RadonLabs GmbH

namespace IO
{
	typedef Ptr<class IStream> PStream;
}

namespace Video
{

class COGGTheoraPlayer: public CVideoPlayer
{
protected:

	ogg_sync_state		oy;
	ogg_page			og;
	ogg_stream_state	vo;
	ogg_stream_state	to;
	theora_info			ti;
	theora_comment		tc;
	theora_state		td;

	int					theora_p;
	int					stateflag;

	/* single frame video buffering */
	int					videobuf_ready;
	ogg_int64_t			videobuf_granulepos;
	double				videobuf_time;
	int					frameNr;

	ogg_packet			op;
	IO::PStream			infile;
	yuv_buffer			yuv;

	unsigned char*		rgbBuffer;
	int					fileVideoDataOffset;
	bool				theoraLoaded;
	bool				isPlaying;
	CTime				currentTime;
	UPTR				decodedFrames;

	int buffer_data(IO::IStream *in,ogg_sync_state *oy);
	int queue_page(ogg_page *page);
	void DecodeYUV(yuv_buffer& yuv,unsigned char* rgbBuffer);
	void StopTheora();
	void UpdateTexture();

public:

	COGGTheoraPlayer();
	//virtual ~COGGTheoraPlayer();

	virtual bool Open();
	virtual void Close();
	virtual void DecodeNextFrame();
	virtual void Decode(CTime DeltaTime);
	virtual void Rewind();
};

}

#endif

