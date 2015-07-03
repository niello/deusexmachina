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

	void	Set(DWORD Flag) { Flags |= Flag; } //!!!can return old value!
	void	SetTo(DWORD Flag, bool Value) { if (Value) Flags |= Flag; else Flags &= ~Flag; } //!!!do it better!
	void	SetAll() { Flags = (DWORD)-1; }
	void	ResetTo(DWORD NewFlags) { Flags = NewFlags; }
	void	Clear(DWORD Mask) { Flags &= ~Mask; }
	void	ClearAll() { Flags = (DWORD)0; }
	void	Invert(DWORD Mask) { Flags ^= Mask; }
	void	InvertAll() { Flags = ~Flags; }
	bool	Is(DWORD Mask) const { return (Flags & Mask) == Mask; }
	bool	IsNot(DWORD Mask) const { return (Flags & Mask) == 0; }
	bool	IsAny(DWORD Mask) const { return (Flags & Mask) != 0; }
	bool	IsAll() const { return Flags == ((DWORD)-1); }
	bool	IsNotAll() const { return Flags == 0; }
	DWORD	GetMask() const { return Flags; }
	DWORD	NumberOfSetBits() const;

	operator DWORD() const { return Flags; }
};

// Not tested, I copied it from
// http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
//???is usable for non-32-bit flags? if less, will be adjusted with 0 bits, but what if more?
inline DWORD CFlags::NumberOfSetBits() const
{
	DWORD Tmp = Flags - ((Flags >> 1) & 0x55555555);
	Tmp = (Tmp & 0x33333333) + ((Tmp >> 2) & 0x33333333);
	return (((Tmp + (Tmp >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
//---------------------------------------------------------------------

}

#endif
