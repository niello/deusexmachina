#include "Stream.h"

namespace Data
{

// NB: it doesn't set IS_OPEN flag, derived class must set it on success
bool CStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpen());
	if ((Mode & SAM_READ) && !CanRead()) FAIL;
	if (((Mode & SAM_WRITE) || (Mode & SAM_APPEND)) && !CanWrite()) FAIL;
	AccessMode = Mode;
	AccessPattern = Pattern;
	OK;
}
//---------------------------------------------------------------------

}