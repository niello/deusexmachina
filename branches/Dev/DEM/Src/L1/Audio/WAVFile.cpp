#include "WAVFile.h"

#include <IO/Streams/FileStream.h>
#include <IO/IOServer.h>

namespace Audio
{
bool CWAVFile::mmioProcInstalled = false;

// This is a custom file access hook for the Win32 MMIO subsystem. It
// redirects all file accesses to the Nebula2 file server, and thus
// makes audio files inside an NPK archive accessible for the MMIO subsystem
LRESULT CALLBACK mmioProc(LPSTR lpstr, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    MMIOINFO* lpMMIOInfo = (MMIOINFO*)lpstr;

    n_assert(0 != lpMMIOInfo);
    n_assert(mmioFOURCC('N','E','B','2') == lpMMIOInfo->fccIOProc);
    n_assert(mmioProc == lpMMIOInfo->pIOProc);

    switch (uMsg)
    {
        case MMIOM_OPEN:
            {
                if (0 == (lpMMIOInfo->dwFlags & (MMIO_WRITE | MMIO_READWRITE)))
                {
                    // strip the "X.NEB2+" prefix from the file
                    CString rawName((const char*)lParam1);
                    CString FileName = rawName.SubString(7, rawName.Length() - 7);

                    // open a Nebula2 file for reading
					IO::CFileStream* file = n_new(IO::CFileStream);
					if (file->Open(FileName, IO::SAM_READ))
                    {
                        // store nebula file object in mmio struct
                        lpMMIOInfo->adwInfo[0] = (DWORD)file;
                        return MMSYSERR_NOERROR;
                    }
                    else n_delete(file);
                }

                // fallthrough: some error occurred
                return MMIOERR_CANNOTOPEN;
            }

        case MMIOM_CLOSE:
            {
                IO::CFileStream* file = (IO::CFileStream*)lpMMIOInfo->adwInfo[0];
                n_assert(0 != file);

                file->Close();
                n_delete(file);
                lpMMIOInfo->adwInfo[0] = 0;

                return 0;
            }

        case MMIOM_READ:
            {
                IO::CFileStream* file = (IO::CFileStream*)lpMMIOInfo->adwInfo[0];
                n_assert(0 != file && file->IsOpen());

                int bytesRead = file->Read((HPSTR)lParam1, (LONG)lParam2);
                lpMMIOInfo->lDiskOffset += bytesRead;

                return bytesRead;
            }

        case MMIOM_SEEK:
            {
                IO::CFileStream* file = (IO::CFileStream*)lpMMIOInfo->adwInfo[0];
                n_assert(0 != file && file->IsOpen());

				IO::ESeekOrigin seekType;
                switch ((int)lParam2)
                {
					case SEEK_CUR: seekType = IO::Seek_Current; break;
					case SEEK_END: seekType = IO::Seek_End; break;
					default:       seekType = IO::Seek_Begin; break;
                }

                if (file->Seek((LONG)lParam1, seekType))
                {
                    lpMMIOInfo->lDiskOffset = file->GetPosition();
                    return lpMMIOInfo->lDiskOffset;
                }
                else
                {
                    return -1;
                }
            }

        case MMIOM_WRITE:
        case MMIOM_WRITEFLUSH:
            // not supported
            return -1;
    }

    // message not recognized
    return 0;
}
//------------------------------------------------------------------------------

CWAVFile::CWAVFile(): m_pwfx(NULL), m_hmmio(NULL), Size(0)
{
	if (!mmioProcInstalled)
	{
		n_assert(mmioInstallIOProc(mmioFOURCC('N','E','B','2'), mmioProc, MMIO_INSTALLPROC));
		mmioProcInstalled = true;
	}
}
//---------------------------------------------------------------------

bool CWAVFile::Open(const CString& FileName)
{
	n_assert(!_IsOpen);
	n_assert(FileName.IsValid());

	// Modify FileName so that MMIO will invoke our custom file function
	CString Path = IOSrv->ManglePath(FileName);
	CString MMIOFileName("X.NEB2+");
	MMIOFileName.Add(Path);

	m_hmmio = mmioOpen((LPSTR)MMIOFileName.CStr(), NULL, MMIO_ALLOCBUF | MMIO_READ);
	if (!m_hmmio)
	{
		Core::Error("CWAVFile::Open(): failed to open file '%s'!", FileName.CStr());
		return false;
	}

	if (!ReadMMIO())
	{
		mmioClose(m_hmmio, 0);
		Core::Error("CWAVFile::Open(): not a wav file (%s)!", FileName.CStr());
		return false;
	}

	Reset();
	Size = m_ck.cksize;

	_IsOpen = true;
	return true;
}
//---------------------------------------------------------------------

void CWAVFile::Close()
{
	n_assert(_IsOpen);
	n_assert(m_hmmio);

	n_delete(m_pwfx);
	m_pwfx = NULL;

	mmioClose(m_hmmio, 0);
	m_hmmio = NULL;
	_IsOpen = false;
}
//---------------------------------------------------------------------

// Read the WAV file header data and other wav specific stuff
bool CWAVFile::ReadMMIO()
{
	n_assert(m_hmmio);

	MMCKINFO ckIn;
	PCMWAVEFORMAT pcmWaveFormat;

	m_pwfx = NULL;

	if (mmioDescend(m_hmmio, &m_ckRiff, NULL, 0) != 0) return false;
	if ((m_ckRiff.ckid != FOURCC_RIFF) || (m_ckRiff.fccType != mmioFOURCC('W','A','V','E'))) return false;

	ckIn.ckid = mmioFOURCC('f','m','t',' ');
	if (mmioDescend(m_hmmio, &ckIn, &m_ckRiff, MMIO_FINDCHUNK) != 0) return false;

	// Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
	// if there are extra parameters at the end, we'll ignore them
	if (ckIn.cksize < (LONG)sizeof(PCMWAVEFORMAT)) return false;

	// read fmt chunk into pcmWaveFormat
	if (mmioRead(m_hmmio, (HPSTR)&pcmWaveFormat, sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat)) return false;

	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
	{
		m_pwfx = (WAVEFORMATEX*) n_new(char[sizeof(WAVEFORMATEX)]);
		n_assert(m_pwfx);
		memcpy(m_pwfx, &pcmWaveFormat, sizeof(pcmWaveFormat));
		m_pwfx->cbSize = 0;
	}
	else
	{
		WORD cbExtraBytes = 0L;
		if (mmioRead(m_hmmio, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD)) return false;

		m_pwfx = (WAVEFORMATEX*)n_new(char[sizeof(WAVEFORMATEX) + cbExtraBytes]);
		n_assert(m_pwfx);
		memcpy(m_pwfx, &pcmWaveFormat, sizeof(pcmWaveFormat));
		m_pwfx->cbSize = cbExtraBytes;

		if (mmioRead(m_hmmio, (CHAR*)(((BYTE*)&(m_pwfx->cbSize)) + sizeof(WORD)), cbExtraBytes) != cbExtraBytes)
		{
			n_delete(m_pwfx);
			m_pwfx = NULL;
			return false;
		}
	}

	if (mmioAscend(m_hmmio, &ckIn, 0) != 0)
	{
		n_delete(m_pwfx);
		m_pwfx = NULL;
		return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool CWAVFile::Reset()
{
	n_assert(m_hmmio);
	if (mmioSeek(m_hmmio, m_ckRiff.dwDataOffset + sizeof(FOURCC), SEEK_SET) == -1) return false;
	m_ck.ckid = mmioFOURCC('d','a','t','a');
	return mmioDescend(m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK) == 0;
}
//---------------------------------------------------------------------

// Reads section of data from a wave file into pBuffer and returns how much read in pdwSizeRead,
// reading not more than dwSizeToRead. This uses m_ck to determine where to start reading from.
// So subsequent calls will be continue where the last left off unless Reset() is called.
uint CWAVFile::Read(void* pBuffer, uint BytesToRead)
{
	n_assert(_IsOpen && m_hmmio && pBuffer && BytesToRead > 0);

	MMIOINFO mmioInfoIn;
	mmioGetInfo(m_hmmio, &mmioInfoIn, 0);

	uint cbDataIn = BytesToRead;
	if (cbDataIn > m_ck.cksize) cbDataIn = m_ck.cksize;
	m_ck.cksize -= cbDataIn;

	for (DWORD cT = 0; cT < cbDataIn; cT++)
	{
		if (mmioInfoIn.pchNext == mmioInfoIn.pchEndRead)
			mmioAdvance(m_hmmio, &mmioInfoIn, MMIO_READ);
		*((uchar*)pBuffer + cT) = *((uchar*)mmioInfoIn.pchNext);
		mmioInfoIn.pchNext++;
	}
	mmioSetInfo(m_hmmio, &mmioInfoIn, 0);
	return cbDataIn;
}
//---------------------------------------------------------------------

}