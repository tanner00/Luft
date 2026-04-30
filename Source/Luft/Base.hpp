#pragma once

using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using usize = uint64;

using bool32 = uint32;

using float32 = float;
using float64 = double;

#define INT8_MIN (-127 - 1)
#define INT16_MIN (-32767 - 1)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN (-9223372036854775807 - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295
#define UINT64_MAX 18446744073709551615

static_assert(sizeof(usize) == sizeof(uint64));
#define USIZE_MAX UINT64_MAX

#define FLOAT32_MAX 3.40282e+38f
#define FLOAT64_MAX 1.79769e+308
#define INFINITY (FLOAT64_MAX * 2.0)

#define INDEX_NONE (static_cast<usize>(~0))

#define ARRAY_COUNT(a) (sizeof((a)) / sizeof((a)[0]))

#define KB(x) ((x) * 1024ull)
#define MB(x) (KB(x) * 1024ull)
#define GB(x) (MB(x) * 1024ull)

#define TOKEN_PASTE_(a, b) a##b
#define TOKEN_PASTE(a, b) TOKEN_PASTE_(a, b)

template<typename T>
void Swap(T& a, T& b)
{
	const T swap = a;
	a = b;
	b = swap;
}
