#pragma once
#ifndef __DEM_L1_FLAGS_H__
#define __DEM_L1_FLAGS_H__

#include <StdDEM.h>

// Helper structure to manage bit flags. Good for state representation.

namespace Data
{

class CFlags //!!!can make template for 8, 16, 32, 64 bit capacity!
{
protected:

	DWORD Flags;

public:

	CFlags(): Flags(0) {}
	CFlags(DWORD InitialFlags): Flags(InitialFlags) {}

	void Set(DWORD Flag) { Flags |= Flag; } //!!!can return old value!
	void SetTo(DWORD Flag, bool Value) { if (Value) Flags |= Flag; else Flags &= ~Flag; } //!!!do it better!
	void SetAll() { Flags = (DWORD)-1; }
	void ResetTo(DWORD Flag) { Flags = Flag; }
	void Clear(DWORD Flag) { Flags &= ~Flag; }
	void ClearAll() { Flags = (DWORD)0; }
	//void Invert(DWORD Flag) { Flags ^= ~Flag; } ???
	bool Is(DWORD Flag) const { return (Flags & Flag) != 0; }
	bool IsNot(DWORD Flag) const { return (Flags & Flag) == 0; }
	//!!!IsAll(DWORD FlagCombination), IsAny(DWORD FlagCombination)!

	//!!!GetSetCount()
};

}

#endif
