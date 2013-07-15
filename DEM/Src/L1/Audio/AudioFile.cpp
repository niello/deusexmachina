#include "AudioFile.h"

namespace Audio
{

// Open for reading & read header
bool CAudioFile::Open(const CString& FileName)
{
	n_assert(!_IsOpen);
	_IsOpen = true;
	return true;
}
//---------------------------------------------------------------------

void CAudioFile::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

// Read data from file. Returns the number of bytes actually read. If decoding
// happens inside Read() the number of bytes to read and number of bytes actually
// read means "decoded data". The method should wrap around if the end of
// the data is encountered.
uint CAudioFile::Read(void* buffer, uint bytesToRead)
{
	n_assert(_IsOpen);
	return 0;
}
//---------------------------------------------------------------------

// Set to the data beginning
bool CAudioFile::Reset()
{
	n_assert(_IsOpen);
	return true;
}
//---------------------------------------------------------------------

int CAudioFile::GetSize() const
{
	n_assert(_IsOpen);
	return 0;
}
//---------------------------------------------------------------------

}