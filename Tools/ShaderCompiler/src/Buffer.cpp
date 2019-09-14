#include "Buffer.h"
#include <string.h>

namespace Data
{
DEFINE_TYPE(CBuffer, CBuffer())

int CBuffer::BinaryCompare(const CBuffer& Other) const
{
	if (DataSize == Other.DataSize)
		return DataSize ? memcmp(pData, Other.pData, DataSize) : 0;
	else if (DataSize > Other.DataSize)
		return 1;
	else
		return -1;
}
//---------------------------------------------------------------------

}