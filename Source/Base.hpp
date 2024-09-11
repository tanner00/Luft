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

#define ARRAY_COUNT(a) (sizeof((a)) / sizeof(*(a)))

#define INDEX_NONE (~static_cast<usize>(0))

#define TOKEN_PASTE_(a, b) a##b
#define TOKEN_PASTE(a, b) TOKEN_PASTE_(a, b)
