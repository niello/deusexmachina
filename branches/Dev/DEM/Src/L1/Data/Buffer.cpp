#include "Buffer.h"
#include <string.h>

namespace Data
{
DEFINE_TYPE(CBuffer, CBuffer())

int CBuffer::BinaryCompare(const CBuffer& Other) const
{
	n_assert(pData && Other.pData);
	if (DataSize == Other.DataSize) return memcmp(this->pData, Other.pData, this->DataSize);
	else if (DataSize > Other.DataSize) return 1;
	else return -1;
}
//---------------------------------------------------------------------

}