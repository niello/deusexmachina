#include "TextReader.h"

namespace IO
{

//!!!TEST different cases, especially last line reading!
bool CTextReader::ReadLine(char* pOutValue, UPTR MaxLen)
{
	const int READ_BLOCK_SIZE = 64;

	if (Stream.IsEOF() || MaxLen < 2) FAIL;

	U64 StartPos = Stream.GetPosition();

	char* pCurr = pOutValue;
	char* pEnd = pOutValue + MaxLen - 1;
	do
	{
		int BytesToRead = pEnd - pCurr;
		if (BytesToRead > READ_BLOCK_SIZE) BytesToRead = READ_BLOCK_SIZE;
		BytesToRead = Stream.Read(pCurr, BytesToRead);

		while (BytesToRead > 0)
		{
			if (*pCurr == '\n' || *pCurr == '\r')
			{
				char Pair = (*pCurr == '\n') ? '\r' : '\n';
				*pCurr = 0;
				++pCurr;
				U64 SeekTo = StartPos + (pCurr - pOutValue);
				if (*pCurr == Pair)
					++SeekTo;
				Stream.Seek(SeekTo, Seek_Begin);
				OK;
			}
			++pCurr;
			--BytesToRead;
		}
	}
	while (pCurr != pEnd && !Stream.IsEOF());

	*pEnd = 0;

	OK; // Else EOF is reached
}
//---------------------------------------------------------------------

}