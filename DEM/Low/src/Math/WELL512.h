#pragma once
#include <StdDEM.h>
#include <random>

// WELL512 random number generator

namespace Math
{

class CWELL512
{
private:

	U32 _State[16];
	U32 _Index = 0;

public:

	using result_type = U32;

	CWELL512(U32 Seed = std::random_device{}())
	{
		std::mt19937 SourceRNG{ Seed };
		for (U32 i = 0; i < 16; ++i)
			_State[i] = SourceRNG();
	}

	// Code from http://lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
	DEM_FORCE_INLINE result_type operator ()() noexcept
	{
		U32 a = _State[_Index];
		U32 c = _State[(_Index + 13) & 15];
		const U32 b = a ^ c ^ (a << 16) ^ (c << 15);
		c = _State[(_Index + 9) & 15];
		c ^= (c >> 11);
		a = _State[_Index] = b ^ c;
		const U32 d = a ^ ((a << 5) & 0xDA442D24UL);
		_Index = (_Index + 15) & 15;
		a = _State[_Index];
		const U32 Value = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
		_State[_Index] = Value;
		return Value;
	}

	static constexpr result_type min() noexcept { return std::numeric_limits<result_type>().min(); }
	static constexpr result_type max() noexcept { return std::numeric_limits<result_type>().max(); }
};

}
