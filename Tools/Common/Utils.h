#pragma once
#include <Data.h> 
#include <algorithm> 
#include <fstream>
#include <vector>
#include <filesystem>

// Utility functions and data structures

struct membuf : std::streambuf
{
	membuf(char* begin, char* end)
	{
		setg(begin, begin, end);
	}
};

// Code from https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
template <typename T>
void apply_permutation_in_place(std::vector<T>& vec, const std::vector<size_t>& p)
{
	std::vector<bool> done(vec.size());
	for (size_t i = 0; i < vec.size(); ++i)
	{
		if (done[i]) continue;
		done[i] = true;
		size_t prev_j = i;
		size_t j = p[i];
		while (i != j)
		{
			std::swap(vec[prev_j], vec[j]);
			done[j] = true;
			prev_j = j;
			j = p[j];
		}
	}
}
//---------------------------------------------------------------------

template <typename T>
void apply_permutation(std::vector<T>& vec, const std::vector<size_t>& p)
{
	std::vector<T> sorted_vec(vec.size());
	std::transform(p.begin(), p.end(), sorted_vec.begin(), [&vec](size_t i) { return vec[i]; });
	std::swap(vec, sorted_vec);
}
//---------------------------------------------------------------------

std::vector<std::string> SplitString(const std::string& Str, char Sep);
uint32_t CalcCRC(const uint8_t* pData, size_t Size);

inline float ByteToNormalizedFloat(uint8_t value)
{
	return value / 255.0f;
}
//---------------------------------------------------------------------

inline uint8_t NormalizedFloatToByte(float value)
{
	return static_cast<uint8_t>(value * 255.0f + 0.5f);
}
//---------------------------------------------------------------------

inline float ShortToNormalizedFloat(uint16_t value)
{
	return value / 65535.0f;
}
//---------------------------------------------------------------------

inline uint16_t NormalizedFloatToShort(float value)
{
	return static_cast<uint16_t>(value * 65535.0f + 0.5f);
}
//---------------------------------------------------------------------

constexpr inline uint32_t ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	return ((uint32_t)r) | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}
//---------------------------------------------------------------------

constexpr inline uint32_t ColorRGBANorm(float r, float g, float b, float a = 1.f)
{
	return ColorRGBA(NormalizedFloatToByte(r), NormalizedFloatToByte(g), NormalizedFloatToByte(b), NormalizedFloatToByte(a));
}
//---------------------------------------------------------------------

template<typename T>
bool ReadAllFile(const char* pPath, std::vector<T>& Out, bool Binary = true)
{
	int Mode = std::ios_base::in;
	if (Binary) Mode |= std::ios_base::binary;
	std::ifstream File(pPath, Mode);
	if (!File) return false;

	Out.assign(std::istreambuf_iterator<T>(File), std::istreambuf_iterator<T>());

	return !File.bad();
}
//---------------------------------------------------------------------

inline void EnsurePathHasEndingDirSeparator(std::string& Path)
{
	const size_t PathLen = Path.size();
	if (PathLen && Path[PathLen - 1] != '/') Path += '/';
}
//---------------------------------------------------------------------

template<class T> inline void ReadStream(std::istream& Stream, T& Out)
{
	Stream.read(reinterpret_cast<char*>(&Out), sizeof(T));
}
//---------------------------------------------------------------------

template<>
inline void ReadStream(std::istream& Stream, std::string& Data)
{
	uint16_t Length;
	ReadStream<uint16_t>(Stream, Length);

	if (Length)
	{
		Data.resize(Length, '\0');
		Stream.read(Data.data(), Length);
	}
}
//---------------------------------------------------------------------

// Can be used to skip data
template<class T> inline T ReadStream(std::istream& Stream)
{
	T Out;
	ReadStream(Stream, Out);
	return Out;
}
//---------------------------------------------------------------------

template<class T> inline void WriteStream(std::ostream& Stream, const T& Value)
{
	//static_assert(!std::is_same_v<T, size_t>, "You should not save types of variable size, please cast to uint64_t or uint32_t explicitly");
	Stream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
}
//---------------------------------------------------------------------

inline void WriteText(std::ostream& Stream, const std::string& Value)
{
	Stream.write(Value.c_str(), Value.size());
}
//---------------------------------------------------------------------

inline void WriteText(std::ostream& Stream, const char* Value)
{
	if (Value) Stream.write(Value, strlen(Value));
}
//---------------------------------------------------------------------

template<size_t N>
inline void WriteText(std::ostream& Stream, const char (&Value)[N])
{
	Stream.write(Value, N - 1);
}
//---------------------------------------------------------------------

template<size_t N>
void WriteCharArrayToStream(std::ostream& Stream, const char (&Value)[N])
{
	auto Length = static_cast<uint16_t>(N);
	if (Value[N - 1] == 0) --Length;
	WriteStream<uint16_t>(Stream, Length);
	if (Length) Stream.write(Value, Length);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const std::string& Value)
{
	const auto Length = static_cast<uint16_t>(Value.size());
	WriteStream<uint16_t>(Stream, Length);
	if (Length) Stream.write(Value.c_str(), Length);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const CStrID& Value)
{
	WriteStream(Stream, std::string(Value.CStr() ? Value.CStr() : ""));
}
//---------------------------------------------------------------------

void WriteData(std::ostream& Stream, const Data::CData& Value);

template<>
inline void WriteStream(std::ostream& Stream, const Data::CData& Value)
{
	WriteData(Stream, Value);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const Data::CDataArray& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const auto& Data : Value)
		WriteStream(Stream, Data);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const Data::CParams& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const auto& Pair : Value)
	{
		WriteStream(Stream, Pair.first.ToString());
		WriteStream(Stream, Pair.second);
	}
}
//---------------------------------------------------------------------

template<typename T>
inline void WriteVectorToStream(std::ostream& Stream, const std::vector<T>& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const T& Data : Value)
		WriteStream<T>(Stream, Data);
}
//---------------------------------------------------------------------

template<typename T, typename U>
inline void WriteMapToStream(std::ostream& Stream, const std::map<T, U>& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const auto& [TT, UU] : Value)
	{
		WriteStream<T>(Stream, TT);
		WriteStream<U>(Stream, UU);
	}
}
//---------------------------------------------------------------------

inline std::string FourCC(uint32_t Code)
{
	std::string Result;
	Result.push_back((Code & 0xff000000) >> 24);
	Result.push_back((Code & 0x00ff0000) >> 16);
	Result.push_back((Code & 0x0000ff00) >> 8);
	Result.push_back(Code & 0x000000ff);
	return Result;
}
//---------------------------------------------------------------------

// trim from start (in place)
inline void ltrim(std::string& s, const std::string& whitespace)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&whitespace](int ch)
	{
		return whitespace.find(ch) == std::string::npos;
	}));
}
//---------------------------------------------------------------------

// trim from end (in place)
inline void rtrim(std::string& s, const std::string& whitespace)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [&whitespace](int ch)
	{
		return whitespace.find(ch) == std::string::npos;
	}).base(), s.end());
}
//---------------------------------------------------------------------

// trim from both ends (in place)
inline void trim(std::string& s, const std::string& whitespace)
{
	ltrim(s, whitespace);
	rtrim(s, whitespace);
}
//---------------------------------------------------------------------

inline void ToLower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}
//---------------------------------------------------------------------

inline void ToUpper(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}
//---------------------------------------------------------------------

inline std::string GetValidFileName(const std::string& BaseName)
{
	// TODO: std::transform with replacer callback?
	std::string RsrcName = BaseName;
	std::replace(RsrcName.begin(), RsrcName.end(), ':', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '|', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '?', '_');
	return RsrcName;
}
//---------------------------------------------------------------------

inline std::string GetValidResourceName(const std::string& BaseName)
{
	std::string RsrcName = GetValidFileName(BaseName);
	std::replace(RsrcName.begin(), RsrcName.end(), ' ', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '-', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '.', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '^', '_');
	ToLower(RsrcName);
	return RsrcName;
}
//---------------------------------------------------------------------

inline std::string GetValidNodeName(const std::string& BaseName)
{
	std::string RsrcName = GetValidFileName(BaseName);
	std::replace(RsrcName.begin(), RsrcName.end(), '.', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '^', '_');
	return RsrcName;
}
//---------------------------------------------------------------------

inline std::filesystem::path ResolvePathAliases(const std::string& Path, const std::map<std::string, std::filesystem::path>& Aliases)
{
	std::string Result = Path;
	auto Pos = Result.find_first_of(':');
	while (Pos != std::string::npos)
	{
		if (Pos > 0)
		{
			std::string Alias = Result.substr(0, Pos);
			auto It = Aliases.find(Alias);
			if (It == Aliases.cend()) break; // Not an alias, path is resolved
			Result = (It->second / Result.substr(Pos + 1)).string();
		}
		else Result = Result.substr(1); // Empty alias, just remove leading ':'

		Pos = Result.find_first_of(':');
	}

	return Result;
}
//---------------------------------------------------------------------

template <typename T>
inline constexpr bool IsPow2(T x) noexcept
{
	return x > 0 && (x & (x - 1)) == 0;
}
//---------------------------------------------------------------------

template <typename T>
inline constexpr T NextPow2(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_ceil(x);
#else
	if constexpr (std::is_signed_v<T>)
	{
		if (x < 0) return 0;
	}

	--x;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	if constexpr (sizeof(T) > 4)
		x |= (x >> 32);

	return x + 1;
#endif
}
//---------------------------------------------------------------------

template <typename T>
inline constexpr T PrevPow2(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_floor(x);
#else
	if constexpr (std::is_signed_v<T>)
	{
		if (x <= 0) return 0;
	}
	else
	{
		if (!x) return 0;
	}

	//--x; // Uncomment this to have a strictly less result
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	if constexpr (sizeof(T) > 4)
		x |= (x >> 32);

	return x - (x >> 1);
#endif
}
//---------------------------------------------------------------------

template <typename T>
inline constexpr int CountLeadingZeros(T x) noexcept
{
	static_assert(sizeof(T) <= 8 && std::is_unsigned_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>);

#if __cplusplus >= 202002L
	return std::countl_zero(x);
#elif defined(__GNUC__)
	if constexpr (sizeof(T) == sizeof(unsigned long long))
		return __builtin_clzll(x);
	else if constexpr (sizeof(T) == sizeof(unsigned long))
		return __builtin_clzl(x);
	else
		return __builtin_clz(x);
#elif defined(_MSC_VER)
	unsigned long Bit;
#if DEM_64
	if constexpr (sizeof(T) == 8)
	{
		if (!_BitScanReverse64(&Bit, x)) Bit = -1;
		return 63 - Bit;
	}
	if constexpr (sizeof(T) == 4)
	{
		_BitScanReverse64(&Bit, static_cast<uint64_t>(x) * 2 + 1);
		return 32 - Bit;
	}
#else
	if constexpr (sizeof(T) == 8)
	{
		int Bits = 64;
		while (x)
		{
			--Bits;
			x >>= 1;
		}
		return Bits;
	}
	if constexpr (sizeof(T) == 4)
	{
		if (!_BitScanReverse(&Bit, x)) Bit = -1;
		return 31 - Bit;
	}
#endif
	if constexpr (sizeof(T) < 4)
	{
		_BitScanReverse(&Bit, static_cast<uint32_t>(x) * 2 + 1);
		return static_cast<T>(sizeof(T) * 8 - Bit);
	}
#else
	int Bits = sizeof(T) * 8;
	while (x)
	{
		--Bits;
		x >>= 1;
	}
	return Bits;
#endif
}
//---------------------------------------------------------------------

template <typename T>
inline constexpr int BitWidth(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_width(x);
#else
	return std::numeric_limits<T>::digits - CountLeadingZeros(x);
#endif
}
//---------------------------------------------------------------------

inline uint32_t Log2(uint32_t x)
{
	uint32_t r = 0;
	while (x >>= 1) ++r;
	return r;
}
//---------------------------------------------------------------------

// Divide and round up
template <typename T, typename U, typename = std::enable_if_t<std::is_integral_v<T> && std::is_integral_v<U>>>
inline decltype(auto) DivCeil(T Numerator, U Denominator)
{
	return (Numerator + Denominator - 1) / Denominator;
}
//---------------------------------------------------------------------

// Returns the size in the same elements in which Width and Height are expressed.
inline uint32_t CalcSizeWithMips(uint32_t Width, uint32_t Height, uint32_t LODCount, bool StopAt1x1 = true)
{
	uint32_t Total = 0;

	if (Width && Height)
	{
		uint32_t CurrW = Width;
		uint32_t CurrH = Height;
		for (uint32_t LOD = 0; LOD < LODCount; ++LOD)
		{
			Total += CurrW * CurrH;

			if (StopAt1x1 && CurrW == 1 && CurrH == 1) break;

			CurrW = DivCeil(CurrW, 2);
			CurrH = DivCeil(CurrH, 2);
		}
	}

	return Total;
}
//---------------------------------------------------------------------
