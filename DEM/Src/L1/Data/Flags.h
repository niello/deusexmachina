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

	U32 Flags;

public:

	CFlags(): Flags(0) {}
	CFlags(U32 InitialFlags): Flags(InitialFlags) {}

	void	Set(U32 Flag) { Flags |= Flag; } //!!!can return old value!
	void	SetTo(U32 Flag, bool Value) { if (Value) Flags |= Flag; else Flags &= ~Flag; } //!!!do it better!
	void	SetAll() { Flags = (U32)-1; }
	void	ResetTo(U32 NewFlags) { Flags = NewFlags; }
	void	Clear(U32 Mask) { Flags &= ~Mask; }
	void	ClearAll() { Flags = (U32)0; }
	void	Invert(U32 Mask) { Flags ^= Mask; }
	void	InvertAll() { Flags = ~Flags; }
	bool	Is(U32 Mask) const { return (Flags & Mask) == Mask; }
	bool	IsNot(U32 Mask) const { return (Flags & Mask) == 0; }
	bool	IsAny() const { return !!Flags; }
	bool	IsAny(U32 Mask) const { return (Flags & Mask) != 0; }
	bool	IsAll() const { return Flags == ((U32)-1); }
	bool	IsNotAll() const { return Flags == 0; }
	U32		GetMask() const { return Flags; }
	U32		NumberOfSetBits() const;

	operator U32() const { return Flags; }
};

// Not tested, I copied it from
// http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
//???is usable for non-32-bit flags? if less, will be adjusted with 0 bits, but what if more?
inline U32 CFlags::NumberOfSetBits() const
{
	DWORD Tmp = Flags - ((Flags >> 1) & 0x55555555);
	Tmp = (Tmp & 0x33333333) + ((Tmp >> 2) & 0x33333333);
	return (((Tmp + (Tmp >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
//---------------------------------------------------------------------

}

#endif
