#include "OGGFile.h"

#include <IO/Streams/FileStream.h>

namespace Audio
{

bool COGGFile::Open(const nString& FileName)
{
	n_assert(!pStream);
	pStream = n_new(IO::CFileStream);
	return ((IO::CFileStream*)pStream)->Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL) && InitOGG();
}
//---------------------------------------------------------------------

void COGGFile::Close()
{
	n_assert(pStream);
	ReleaseOGG();
	pStream->Close();
	n_delete(pStream);
	pStream = NULL;
}
//---------------------------------------------------------------------

bool COGGFile::Reset()
{
	n_assert(pStream);
	return ReleaseOGG() && pStream->Seek(0, IO::Seek_Begin) && InitOGG();
}
//---------------------------------------------------------------------

uint COGGFile::Read(void* pBuffer, uint BytesToRead)
{
	n_assert(pStream && pBuffer && BytesToRead > 0);

	memset(pBuffer, 0, BytesToRead);

	int samplesOut = 0;
	int samplesWritten = 0;
	int samplesLeft = BytesToRead / ( 2 * vBitStreamInfo.channels);

	ogg_int16_t* bufferOut = (ogg_int16_t*)pBuffer;

	// Straight decode loop until end of stream
	while (!endOfStream)
	{
        while (!endOfStream)
        {
            int Result = 1;

            if (!readEarlyLoopExit) Result = ogg_sync_pageout(&oSyncState, &oBitStreamPage);
            if (Result == 0) break;
            if (Result < 0) // need more data
            {
				n_printf("OGG: Corrupt or missing data in bitstream; continuing...\n");
            }
            if (Result > 0)
            {
                if (!readEarlyLoopExit)
                {
                    /* can safely ignore errors at this point */
                    ogg_stream_pagein(&oStreamState, &oBitStreamPage);
                }

                while (1)
                {
                    int Result=1;

                    if (!readEarlyLoopExit)
                    {
                        Result = ogg_stream_packetout(&oStreamState, &oRawPacket);
                    }

                    if (Result == 0)
                    {
                        break;
                    }

                    /* need more data */
                    if (Result < 0)
                    {
                        /* missing or corrupt data at this page position */
                        /* no reason to complain; already complained above */
                    }
                    if (Result > 0)
                    {
                        /* we have a packet.  Decode it */

                        if (!readEarlyLoopExit)
                        {
                            if (vorbis_synthesis(&vDecoderWorkSpace, &oRawPacket) == 0)
                            /* test for success! */
                            {
                                vorbis_synthesis_blockin(&vDecoderState, &vDecoderWorkSpace);
                            }
                        }
                        /*
                        **pcm is a multichannel float vector.  In stereo, for
                        example, pcm[0] is left, and pcm[1] is right.  samples is
                        the size of each channel.  Convert the float values
                        (-1.<=range<=1.) to whatever PCM format and write it out */
                        float **pcm;
                        int samples = 0;
                        while ((samples = vorbis_synthesis_pcmout(&vDecoderState, &pcm)) > 0)
                        {
                            int i,j;
                            int clipFlag = 0;
                            samplesOut = (samples < samplesLeft ? samples : samplesLeft);

                            /* convert floats to 16 bit signed ints (host order)
                            and interleave */
                            for (i = 0; i < vBitStreamInfo.channels; i++)
                            {
                                ogg_int16_t *ptr = bufferOut + samplesWritten*vBitStreamInfo.channels + i;
                                float *mono = pcm[i];
                                for (j = 0; j < samplesOut; j++)
                                {
                                    int val = (int)(mono[j] *32767.f);
                                    /* might as well guard against clipping */
                                    if (val > 32767)
                                    {
                                        val = 32767;
                                        clipFlag = 1;
                                    }
                                    if (val <  - 32768)
                                    {
                                        val =  - 32768;
                                        clipFlag = 1;
                                    }

                                    *ptr = val;
                                    ptr += vBitStreamInfo.channels;
                                }
                            }

                            if (clipFlag)
                            {
                                // FIXME Floh: is this a serious error?
                                // n_printf("Clipping in frame %ld\n", (long)(vDecoderState.sequence));
                            }

                            samplesWritten+= samplesOut ;
                            samplesLeft   -= samplesOut ;

                            /* tell libvorbis how many samples we actually consumed */
                            vorbis_synthesis_read(&vDecoderState, samplesOut);

                            if (samplesLeft <= 0)
                            {
                                readEarlyLoopExit=true;
                                int byteSize=2*vBitStreamInfo.channels*samplesWritten;
                                return byteSize;
                            }
                            readEarlyLoopExit=false;
                        }
                        readEarlyLoopExit=false;
                    }
                }

                if (ogg_page_eos(&oBitStreamPage))
                {
                    endOfStream = true;
                }
            }
        }
        if (!endOfStream)
        {
            bufferIn = ogg_sync_buffer(&oSyncState, INPUTBUFFER);
            bytesIn = pStream->Read(bufferIn, INPUTBUFFER);
            ogg_sync_wrote(&oSyncState, bytesIn);


            if (bytesIn == 0)
            {
                endOfStream = true;
            }

        }
	}

	m_bFileEndReached = true;

	// maybe there is still some data left which doesnt fill a block
	if (samplesWritten != 0) return BytesToRead;

	if (keepAlive > 0)
	{
		// fake that pBuffer got filled
		keepAlive--;
		return BytesToRead;
	};

	return 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
COGGFile::InitOGG()
{
    n_assert(0 != pStream);

    readEarlyLoopExit=false;
    endOfStream=false;
    m_bFileEndReached = false;
    keepAlive = 1;

    size=0;

    /********** Decode setup ************/

    memset (&oSyncState,0,sizeof(ogg_sync_state));
    memset (&oStreamState,0,sizeof(ogg_stream_state));
    memset (&oBitStreamPage,0,sizeof(ogg_page));
    memset (&oRawPacket,0,sizeof(ogg_packet));

    memset (&vBitStreamInfo,0,sizeof(vorbis_info));
    memset (&vComments,0,sizeof(vorbis_comment));
    memset (&vDecoderState,0,sizeof(vorbis_dsp_state));
    memset (&vDecoderWorkSpace,0,sizeof(vorbis_block));

    ogg_sync_init(&oSyncState); /* Now we can read pages */

    /* grab some data at the head of the stream.  We want the first page
    (which is guaranteed to be small and only contain the Vorbis
    stream initial header) We need the first page to get the stream
    serialno. */

    /* submit a 4k block to libvorbis' Ogg layer */
    bufferIn = ogg_sync_buffer(&oSyncState, INPUTBUFFER);
    bytesIn = pStream->Read(bufferIn,  INPUTBUFFER);
    ogg_sync_wrote(&oSyncState, bytesIn);

    /* Get the first page. */
    if (ogg_sync_pageout(&oSyncState, &oBitStreamPage) != 1)
    {
        /* have we simply run out of data?  If so, we're done. */
        if (bytesIn < INPUTBUFFER)
        {
            /* OK, clean up the framer */
            ogg_sync_clear(&oSyncState);

            pStream->Close();

            n_printf("COggFile::Open:Done.\n");
        }

        /* error case.  Must not be Vorbis data */
        n_printf("COggFile::Open:Input does not appear to be an Ogg bitstream.\n");
        return false;
    }

    /* Get the serial number and set up the rest of decode. */
    /* serialno first; use it to set up a logical stream */
    ogg_stream_init(&oStreamState, ogg_page_serialno(&oBitStreamPage));

    /* extract the initial header from the first page and verify that the
    Ogg bitstream is in fact Vorbis data */

    /* I handle the initial header first instead of just having the code
    read all three Vorbis headers at once because reading the initial
    header is an easy way to identify a Vorbis bitstream and it's
    useful to see that functionality seperated out. */

    vorbis_info_init(&vBitStreamInfo);
    vorbis_comment_init(&vComments);
    if (ogg_stream_pagein(&oStreamState, &oBitStreamPage) < 0)
    {
        /* error; stream version mismatch perhaps */
        n_printf("COggFile::Open:Error reading first page of Ogg bitstream data.\n");
        return false;
    }

    if (ogg_stream_packetout(&oStreamState, &oRawPacket) != 1)
    {
        /* no page? must not be vorbis */
        n_printf("COggFile::Open:Error reading initial header packet.\n");
        return false;
    }

    if (vorbis_synthesis_headerin(&vBitStreamInfo, &vComments, &oRawPacket) < 0)
    {
        /* error case; not a vorbis header */
        n_printf("COggFile::Open:This Ogg bitstream does not contain Vorbis "
        "audio data.\n");
        return false;
    }

    /* At this point, we're sure we're Vorbis.  We've set up the logical
    (Ogg) bitstream decoder.  Get the comment and codebook headers and
    set up the Vorbis decoder */

    /* The next two packets in order are the comment and codebook headers.
    They're likely large and may span multiple pages.  Thus we reead
    and submit data until we get our two pacakets, watching that no
    pages are missing.  If a page is missing, error out; losing a
    header page is the only place where missing data is fatal. */

    int i=0;

    while (i < 2)
    {
        while (i < 2)
        {
        int Result = ogg_sync_pageout(&oSyncState, &oBitStreamPage);
        if (Result == 0)
        {
            break;
        }

        /* Need more data */
        /* Don't complain about missing or corrupt data yet.  We'll
        catch it at the packet output phase */
        if (Result == 1)
        {
            /* we can ignore any errors here as they'll also become apparent at packetout */
            ogg_stream_pagein(&oStreamState, &oBitStreamPage);
            while (i < 2)
            {
                Result = ogg_stream_packetout(&oStreamState, &oRawPacket);
                if (Result == 0)
                {
                    break;
                }
                if (Result < 0)
                {
                    /* Uh oh; data at some point was corrupted or missing!
                    We can't tolerate that in a header.  Die. */
                    n_printf("COggFile::Open:Corrupt secondary header.  Exiting.\n");
                    return false;
                }
                vorbis_synthesis_headerin(&vBitStreamInfo, &vComments, &oRawPacket);
                i++;
            }
        }
        }
        /* no harm in not checking before adding more */
        bufferIn = ogg_sync_buffer(&oSyncState, INPUTBUFFER);
        bytesIn = pStream->Read(bufferIn, INPUTBUFFER);
        if (bytesIn == 0 && i < 2)
        {
            n_printf("COggFile::Open:End of pStream before finding all Vorbis headers!\n");
            return false;
        }
        ogg_sync_wrote(&oSyncState, bytesIn);
    }

    /* Throw the comments plus a few lines about the bitstream we're
    decoding */
    {
        char **ptr = vComments.user_comments;
        while (*ptr)
        {
            n_printf("COggFile::Open:%s\n",  *ptr);
            ++ptr;
        }
        n_printf("COggFile::Open:\nBitstream is %d channel, %ldHz\n", vBitStreamInfo.channels,
            vBitStreamInfo.rate);
        n_printf("COggFile::Open:Encoded by: %s\n\n", vComments.vendor);

        ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
        wfx.nChannels         = vBitStreamInfo.channels;
        wfx.nSamplesPerSec    = vBitStreamInfo.rate;
        wfx.wBitsPerSample    = 16;
        wfx.nBlockAlign       = wfx.nChannels*wfx.wBitsPerSample/8;
        wfx.nAvgBytesPerSec   = vBitStreamInfo.rate*wfx.nBlockAlign;
        wfx.wFormatTag        = (WORD) WAVE_FORMAT_PCM;
        wfx.cbSize            = 0;//???
    }

    /* OK, got and parsed all three headers.Initialize the Vorbis packet->PCM decoder. */
    vorbis_synthesis_init(&vDecoderState, &vBitStreamInfo); /* central decode state */
    vorbis_block_init(&vDecoderState, &vDecoderWorkSpace);
    /* local state for most of the decode so multiple block decodes can
    proceed in parallel.  We could init multiple vorbis_block structures
    for vDecoderState here */


    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
COGGFile::ReleaseOGG()
{
    /* clean up this logical bitstream; before exit we see if we're
    followed by another [chained] */

    ogg_stream_clear(&oStreamState);

    /* ogg_page and ogg_packet structs always point to storage in
    libvorbis.  They're never freed or manipulated directly */

    vorbis_block_clear(&vDecoderWorkSpace);
    vorbis_dsp_clear(&vDecoderState);
    vorbis_comment_clear(&vComments);
    vorbis_info_clear(&vBitStreamInfo); /* must be called last */

    /* OK, clean up the framer */
    ogg_sync_clear(&oSyncState);

    return true;
}

}