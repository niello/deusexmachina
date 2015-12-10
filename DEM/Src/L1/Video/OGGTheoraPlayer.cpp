#include "OGGTheoraPlayer.h"

#include <IO/Streams/FileStream.h>

namespace Video
{
	
COGGTheoraPlayer::COGGTheoraPlayer():
	rgbBuffer(0),
	isPlaying(false),
	theoraLoaded(false),
	currentTime(0),
	decodedFrames(0)
{
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Helper; just grab some more compressed bitstream and sync it for page extraction
*/
int COGGTheoraPlayer::buffer_data(IO::CFileStream *in,ogg_sync_state *oy)
{
	char *buffer = ogg_sync_buffer(oy, 4096);
	int bytes = (int)in->Read(buffer, 4096);
	ogg_sync_wrote(oy, bytes);
	return bytes;
}

//------------------------------------------------------------------------------
/**
    helper: push a page into the steam for packetization
*/
int
COGGTheoraPlayer::queue_page(ogg_page * /*page*/)
{
    if (theora_p)ogg_stream_pagein(&to,&og);
    return 0;
};

//------------------------------------------------------------------------------
/**
    rewinds the stream
*/
void
COGGTheoraPlayer::Rewind()
{
    if (theoraLoaded)
        StopTheora();

    // go back to the beginning of the file
	infile->Seek(0,IO::Seek_Begin);

    // reset times
    currentTime = 0;
    decodedFrames = 0;

    // initialize theora
    theora_p = 0;
    stateflag = 0;
    videobuf_ready = 0;
    videobuf_granulepos = -1;
    videobuf_time = 0;
    memset(&td,0,sizeof(td));
    /*
        Ok, Ogg parsing. The idea here is we have a bitstream
        that is made up of Ogg pages. The libogg sync layer will
        find them for us. There may be pages from several logical
        streams interleaved; we find the first theora stream and
        ignore any others.

        Then we pass the pages for our stream to the libogg stream
        layer which assembles our original set of packets out of
        them. It's the packets that libtheora actually knows how
        to handle.
    */

    /* start up Ogg stream synchronization layer */
    ogg_sync_init(&oy);

    /* init supporting Theora structures needed in header parsing */
    theora_comment_init(&tc);
    theora_info_init(&ti);

    /* Ogg file open; parse the headers */

    /* Vorbis and Theora both depend on some initial header packets
        for decoder setup and initialization. We retrieve these first
        before entering the main decode loop. */

    /* Only interested in Theora streams */
    while (!stateflag)
    {
        int ret=buffer_data(infile,&oy);
        if (ret==0)break;
        while (ogg_sync_pageout(&oy,&og) > 0)
        {
            ogg_stream_state test;
            /* is this a mandated initial header? If not, stop parsing */
            if (!ogg_page_bos(&og)){
            /* don't leak the page; get it into the appropriate stream */
            queue_page(&og);
            stateflag=1;
            break;
            }
            ogg_stream_init(&test,ogg_page_serialno(&og));
            ogg_stream_pagein(&test,&og);
            ogg_stream_packetout(&test,&op);
            /* identify the codec: try theora */
            if (!theora_p && theora_decode_header(&ti,&tc,&op)>=0){
            /* it is theora -- save this stream state */
            memcpy(&to,&test,sizeof(test));
            theora_p=1;
            }else{
            /* whatever it is, we don't care about it */
            ogg_stream_clear(&test);
            }
        }
        /* fall through to non-initial page parsing */
    }

    /* we're expecting more header packets. */
    while (theora_p && theora_p < 3)
    {
        int ret;

        /* look for further theora headers */
        while (theora_p && (theora_p < 3) && (ret = ogg_stream_packetout(&to,&op)))
        {
            // "Error parsing Theora stream headers; corrupt stream?" ?
            n_assert(ret >= 0);

            // "Error parsing Theora stream headers; corrupt stream?" ?
            n_assert(theora_decode_header(&ti,&tc,&op) == 0);

            theora_p++;
            if (theora_p==3)break;
        }


        /* The header pages/packets will arrive before anything else we
            care about, or the stream is not obeying spec */

        if (ogg_sync_pageout(&oy,&og)>0)
        {
            queue_page(&og); /* demux into the stream state */
        }
        else
        {
            int ret=buffer_data(infile,&oy); /* need more data */
            // "End of file while searching for codec headers." ?
            n_assert(ret != 0);
        }
    }

    /* Now we have all the required headers. initialize the decoder. */
    if (theora_p)
    {
        theora_decode_init(&td,&ti);
        /*
        fprintf(stderr,"Ogg logical stream %x is Theora %dx%d %.02f fps video\nEncoded frame content is %dx%d with %dx%d offset\n",
                to.serialno,ti.width,ti.height, (double)ti.fps_numerator/ti.fps_denominator,
                ti.frame_width, ti.frame_height, ti.offset_x, ti.offset_y);
        */
        videoWidth = ti.frame_width;
        videoHeight = ti.frame_height;
        videoFpS = ((float)ti.fps_numerator/ti.fps_denominator);
    }
    else
    {
        /* tear down the partial theora setup */
        theora_info_clear(&ti);
        theora_comment_clear(&tc);
    }
    /* Finally the main decode loop.

        It's one Theora packet per frame, so this is pretty
        straightforward if we're not trying to maintain sync
        with other multiplexed streams.

        the videobuf_ready flag is used to maintain the input
        buffer in the libogg stream state. If there's no output
        frame available at the end of the decode step, we must
        need more input data. We could simplify this by just
        using the return code on ogg_page_packetout(), but the
        flag system extends easily to the case were you care
        about more than one multiplexed stream (like with audio
        playback). In that case, just maintain a flag for each
        decoder you care about, and pull data when any one of
        them stalls.

        videobuf_time holds the presentation time of the currently
        buffered video frame. We ignore this value.
    */
    stateflag=0; /* playback has not begun */
    /* queue any remaining pages from data we buffered but that did not
        contain headers */
    while (ogg_sync_pageout(&oy,&og) > 0)
    {
        queue_page(&og);
    };
    theoraLoaded = true;
    isPlaying = true;
};

//------------------------------------------------------------------------------
/**
    Shuts down theora decoder
*/
void
COGGTheoraPlayer::StopTheora()
{
    n_assert(theoraLoaded);
    if (theora_p)
    {
        ogg_stream_clear(&to);
        theora_clear(&td);
        theora_comment_clear(&tc);
        theora_info_clear(&ti);
    };
    ogg_sync_clear(&oy);
    isPlaying = false;
};

//------------------------------------------------------------------------------
/**
    opens the player and the file set by SetFilename()
*/
bool
COGGTheoraPlayer::Open()
{
    n_assert(FileName != "");
    // open file
	infile = n_new(IO::CFileStream)(FileName);
	infile->Open(IO::SAM_READ);
    // rewind
    Rewind();
    // setup framebuffer
    rgbBuffer = n_new_array(unsigned char, ti.frame_width*ti.frame_height*4);
    memset(rgbBuffer,255,ti.frame_width*ti.frame_height*4);
    // we don't have a frame yet
    frameNr = -1;
    CVideoPlayer::Open();
    return true;
};

//------------------------------------------------------------------------------
/**
    Decodes a YUV frame into RGB data
*/
void
COGGTheoraPlayer::DecodeYUV(yuv_buffer& yuv,unsigned char* rgbBuffer)
{
    int rgbIndex=0;
    int y;
    for (y = 0; y < yuv.y_height; y++)
    {
        int xsize=yuv.y_width;
        int uvy = (y/2) * yuv.uv_stride;
        int yy = y * yuv.y_stride;
        int x;
        for (x = 0; x < xsize; x++)
        {
            int Y = yuv.y[ yy + x] -16;
            int U = yuv.u[uvy + (x/2)] - 128;
            int V = yuv.v[uvy + (x/2)] - 128;
            int R = ((298*Y         + 409*V + 128)>>8);
            int G = ((298*Y - 100*U - 208*V + 128)>>8);
            int B = ((298*Y + 516*U         + 128)>>8);
            if (R<0) R=0; if (R>255) R=255;
            if (G<0) G=0; if (G>255) G=255;
            if (B<0) B=0; if (B>255) B=255;
            rgbBuffer[rgbIndex++] = (unsigned char)B;
            rgbBuffer[rgbIndex++] = (unsigned char)G;
            rgbBuffer[rgbIndex++] = (unsigned char)R;
            rgbIndex++;
        };
    };
};

//------------------------------------------------------------------------------
/**
    updates the texture-data
*/
void
COGGTheoraPlayer::UpdateTexture()
{
	//GFX
	/*
    n_assert(pTexture != 0);
    // Decode Theora Frame
    theora_decode_YUVout(&td,&yuv);
    DecodeYUV(yuv,rgbBuffer);
    // Setup variables for copying
    int width = pTexture->GetWidth();
    int height = pTexture->GetHeight();
    int line=videoWidth;
    if (line > width) line = width;
    if (height > (int)videoHeight) height = videoHeight;
    // lock texture
    nTexture2::LockInfo info;
    pTexture->Lock(nTexture2::WriteOnly,0,info);
    unsigned char* surfPtr=(unsigned char*)info.surfPointer;
    // update texture
    int i;
    for (i = 0; i < height; i++)
        memcpy(&surfPtr[i*info.surfPitch],&rgbBuffer[i*videoWidth*4],line*4);
    // and unlock it
    pTexture->Unlock(0);
	*/
};

//------------------------------------------------------------------------------
/**
    Decodes the next frame
*/
void
COGGTheoraPlayer::DecodeNextFrame()
{
    n_assert(_IsOpen);
    videobuf_ready = 0;
    while (!videobuf_ready)
    {
        while (theora_p && !videobuf_ready)
        {
            /* theora is one in, one out... */
            if (ogg_stream_packetout(&to,&op) > 0)
            {

                theora_decode_packetin(&td,&op);
                videobuf_granulepos=td.granulepos;
                videobuf_time=theora_granule_time(&td,videobuf_granulepos);
                videobuf_ready=1;

            }else
                break;
        }
        if (!videobuf_ready && infile->IsEOF())
        {
            if (loopType == Repeat)
            {
                Rewind();
            } else {
                // reached end, stop playing
                isPlaying = false;
                return;
            };
        };

        if (!videobuf_ready)
        {
            /* no data yet for somebody.  Grab another page */
            int ret=buffer_data(infile,&oy);
            while (ogg_sync_pageout(&oy,&og) > 0)
            {
                queue_page(&og);
            }
            videobuf_ready = 0;
        };
    };
    if (videobuf_ready == 0)
    {
        return;
    }
    frameNr++;
    decodedFrames++;
    if (DoTextureUpdate)
        UpdateTexture();
    return;
};


//------------------------------------------------------------------------------
/**
    Decodes a timestep
*/
void
COGGTheoraPlayer::Decode(CTime DeltaTime)
{
    n_assert(_IsOpen);
    // calculate how many frames need to be decoded
    currentTime += DeltaTime;
    UPTR neededFrame = (UPTR)(currentTime * (CTime)ti.fps_numerator/(CTime)ti.fps_denominator);
    UPTR framesToDo = neededFrame - decodedFrames;
    // disable automatic updating of the texture
    // we only need to do that once
    bool oldTexUpdate = DoTextureUpdate;
    DoTextureUpdate = false;
    // now decode
    UPTR i;
    for (i = 0; i < framesToDo; i++)
    {
        if (!isPlaying) break;
        DecodeNextFrame();
    };
    // and update once
    DoTextureUpdate = oldTexUpdate;
    if (DoTextureUpdate)
        UpdateTexture();
};

//------------------------------------------------------------------------------
/**
    Closes player and file
*/
void
COGGTheoraPlayer::Close()
{
    /* end of decoder loop -- close everything */
    n_assert(_IsOpen);
    infile->Close();
    n_delete(infile);
	infile = NULL;
    if (theoraLoaded)
        StopTheora();
    n_delete_array(rgbBuffer);
    rgbBuffer = 0;
    _IsOpen = false;
};

}
