#pragma once

#include "Base.hpp"
#include "String.hpp"

inline uint64 HashFnv1a(const void* key, usize keySize)
{
	const uint8* keyBytes = static_cast<const uint8*>(key);

	static constexpr uint64 fnvOffset = 14695981039346656037ull;
	static constexpr uint64 fnvPrime = 1099511628211ull;

	uint64 hash = fnvOffset;
	for (usize i = 0; i < keySize; ++i)
	{
		hash ^= static_cast<uint64>(keyBytes[i]);
		hash *= fnvPrime;
	}
	return hash;
}

template<typename T>
T HashCombine(T hash1, T hash2)
{
	return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

template<typename T, typename... Args>
T HashCombine(T hash1, T hash2, Args... hashes) requires(IsSame<Args, T>::Value && ...)
{
	return HashCombine(HashCombine(hash1, hash2), hashes...);
}

template<typename K>
struct Hash
{
	uint64 operator()(const K& key) const = delete;
};

#define HASH_PRIMITIVE(t)						\
	template<>									\
	struct Hash<t>								\
	{											\
		uint64 operator()(const t& key) const	\
		{										\
			return HashFnv1a(&key, sizeof(t));	\
		}										\
	}

HASH_PRIMITIVE(int8);
HASH_PRIMITIVE(int16);
HASH_PRIMITIVE(int32);
HASH_PRIMITIVE(int64);

HASH_PRIMITIVE(uint8);
HASH_PRIMITIVE(uint16);
HASH_PRIMITIVE(uint32);
HASH_PRIMITIVE(uint64);

HASH_PRIMITIVE(char);

HASH_PRIMITIVE(float32);
HASH_PRIMITIVE(float64);

inline uint64 StringHash(const void* key, usize length)
{
	return HashFnv1a(key, length);
}

template<>
struct Hash<String>
{
	uint64 operator()(const String& key) const
	{
		return StringHash(key.GetData(), key.GetLength());
	}
};

template<>
struct Hash<StringView>
{
	uint64 operator()(StringView key) const
	{
		return StringHash(key.GetData(), key.GetLength());
	}
};

template<typename K, typename H>
concept IsHashable = requires(K key)
{
	Hash<K>{}(key);
};
