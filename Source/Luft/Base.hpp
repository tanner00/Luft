#pragma once

using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using usize = unsigned long long;

using uchar = uint8;

#define ARRAY_COUNT(a) (sizeof((a)) / sizeof((a)[0]))

#define INDEX_NONE (static_cast<usize>(~0))

#define UINT8_MAX (static_cast<uint8>(~0))
#define UINT16_MAX (static_cast<uint16>(~0))
#define UINT32_MAX (static_cast<uint32>(~0))
#define UINT64_MAX (static_cast<uint64>(~0))

#define KB(x) ((x) * 1024ULL)
#define MB(x) (KB(x) * 1024ULL)
#define GB(x) (MB(x) * 1024ULL)

#define TOKEN_PASTE_(a, b) a##b
#define TOKEN_PASTE(a, b) TOKEN_PASTE_(a, b)
