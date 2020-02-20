#include "Stream.h"

namespace IO
{

// NB: it doesn't set IS_OPEN flag, derived class must set it on success
bool IStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpened());
	if ((Mode & SAM_READ) && !CanRead()) FAIL;
	if (((Mode & SAM_WRITE) || (Mode & SAM_APPEND)) && !CanWrite()) FAIL;
	OK;
}
//---------------------------------------------------------------------

}