#pragma once

#include "Allocator.hpp"
#include "Array.hpp"
#include "Hash.hpp"

template<typename BucketArray, typename Pair>
class HashTableIterator
{
public:
	HashTableIterator(BucketArray& buckets, usize valueCount, usize firstBucketIndex)
		: Buckets(buckets)
		, ValueCount(valueCount)
		, InterBucketIndex(firstBucketIndex)
		, IntraBucketIndex(0)
	{
	}

	Pair& operator*() const
	{
		return Buckets[InterBucketIndex][IntraBucketIndex];
	}

	Pair* operator->() const
	{
		return &Buckets[InterBucketIndex][IntraBucketIndex];
	}

	HashTableIterator& operator++()
	{
		++IntraBucketIndex;
		if (IntraBucketIndex == Buckets[InterBucketIndex].GetLength())
		{
			IntraBucketIndex = 0;
			do
			{
				++InterBucketIndex;
			} while (InterBucketIndex != Buckets.GetLength() && Buckets[InterBucketIndex].IsEmpty());
		}
		--ValueCount;
		return *this;
	}

	bool operator==(const HashTableIterator& b)
	{
		return ValueCount == b.ValueCount && &Buckets == &Buckets;
	}

	bool operator!=(const HashTableIterator& b)
	{
		return ValueCount != b.ValueCount || &Buckets != &Buckets;
	}

private:
	BucketArray& Buckets;
	usize ValueCount;
	usize InterBucketIndex;
	usize IntraBucketIndex;
};

template<typename K, typename InputK>
concept IsValidHashTableKey = IsSame<RemoveCvType<InputK>, K>::Value || (IsSame<K, String>::Value && IsSame<RemoveCvType<InputK>, StringView>::Value);

template<typename Pair, typename K, typename InputK>
usize FindPairIndex(const Array<Pair>& bucket, const InputK& key) requires IsValidHashTableKey<K, InputK>
{
	usize index = INDEX_NONE;
	for (usize i = 0; i < bucket.GetLength(); ++i)
	{
		const Pair& currentPair = bucket[i];
		if (key == currentPair.Key)
		{
			index = i;
			break;
		}
	}
	return index;
}

template<typename T>
concept IsEqualable = requires(T a, T b)
{
	a == b;
};

template<typename K, typename V> requires IsEqualable<K> && IsHashable<K, Hash<K>>
class HashTable
{
public:
	struct Pair
	{
		K Key;
		V Value;
	};

	using BucketArray = Array<Array<Pair>>;

	explicit HashTable(usize bucketCount, Allocator* allocator = &GlobalAllocator::Get())
		: ValueCount(0)
		, Allocator(allocator)
	{
		CHECK(bucketCount > 0);
		new (&Buckets, LuftNewMarker {}) BucketArray (bucketCount, Allocator);
		for (usize i = 0; i < bucketCount; ++i)
		{
			Buckets.Emplace(Allocator);
		}
	}

	~HashTable()
	{
		for (Array<Pair>& bucket : Buckets)
		{
			bucket.~Array();
		}
		Buckets.~Array();
		ValueCount = 0;
		Allocator = nullptr;
	}

	HashTable(const HashTable& copy)
		: ValueCount(copy.ValueCount)
		, Allocator(copy.Allocator)
	{
		new (&Buckets, LuftNewMarker {}) BucketArray (copy.Buckets.GetLength(), Allocator);
		for (usize i = 0; i < copy.Buckets.GetLength(); ++i)
		{
			Buckets.Add(copy.Buckets[i]);
		}
	}

	HashTable& operator=(const HashTable& copy)
	{
		CHECK(&copy != this);

		this->~HashTable();

		ValueCount = copy.ValueCount;
		Allocator = copy.Allocator;

		new (&Buckets, LuftNewMarker {}) BucketArray (copy.Buckets.GetLength(), Allocator);
		for (usize i = 0; i < copy.Buckets.GetLength(); ++i)
		{
			Buckets.Add(copy.Buckets[i]);
		}

		return *this;
	}

	HashTable(HashTable&& move) noexcept
		: Buckets(Move(move.Buckets))
		, ValueCount(move.ValueCount)
		, Allocator(move.Allocator)
	{
		move.ValueCount = 0;
		move.Allocator = nullptr;
	}

	HashTable& operator=(HashTable&& move) noexcept
	{
		this->~HashTable();

		Buckets = Move(move.Buckets);
		ValueCount = move.ValueCount;
		Allocator = move.Allocator;

		move.ValueCount = 0;
		move.Allocator = nullptr;

		return *this;
	}

	template<typename InputK>
	V& operator[](const InputK& key) requires IsValidHashTableKey<K, InputK>
	{
		return Get(key);
	}

	template<typename InputK>
	const V& operator[](const InputK& key) const requires IsValidHashTableKey<K, InputK>
	{
		return Get(key);
	}

	usize GetCount() const
	{
		return ValueCount;
	}

	bool IsEmpty() const
	{
		return ValueCount == 0;
	}

	template<typename InputK>
	bool Contains(const InputK& key) const requires IsValidHashTableKey<K, InputK>
	{
		const uint64 bucketIndex = Hash<InputK>{}(key) % Buckets.GetLength();
		return FindPairIndex<Pair, K, InputK>(Buckets[bucketIndex], key) != INDEX_NONE;
	}

	template<typename InputK>
	V& Get(const InputK& key) requires IsValidHashTableKey<K, InputK>
	{
		const uint64 bucketIndex = Hash<InputK>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex<Pair, K, InputK>(bucket, key);
		CHECK(index != INDEX_NONE);
		return bucket[index].Value;
	}

	template<typename InputK>
	const V& Get(const InputK& key) const requires IsValidHashTableKey<K, InputK>
	{
		const uint64 bucketIndex = Hash<InputK>{}(key) % Buckets.GetLength();
		const Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex<Pair, K, InputK>(bucket, key);
		CHECK(index != INDEX_NONE);
		return bucket[index].Value;
	}

	template<typename InputK>
	V& GetOrAdd(const InputK& key) requires IsValidHashTableKey<K, InputK>
	{
		const uint64 bucketIndex = Hash<InputK>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex<Pair, K, InputK>(bucket, key);
		if (index != INDEX_NONE)
		{
			return bucket[index].Value;
		}

		if constexpr (IsSame<RemoveCvType<InputK>, StringView>::Value)
		{
			bucket.Add(Pair { String { key }, {} });
		}
		else
		{
			bucket.Add(Pair { key, {} });
		}
		++ValueCount;

		return bucket[bucket.GetLength() - 1].Value;
	}

	V& GetOrAdd(K&& key)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex<Pair, K, K>(bucket, key);
		if (index != INDEX_NONE)
		{
			return bucket[index].Value;
		}

		bucket.Add(Pair { Move(key), {} });
		++ValueCount;

		return bucket[bucket.GetLength() - 1].Value;
	}

	bool Add(const K& key, const V& value)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize alreadyExistingIndex = FindPairIndex<Pair, K, K>(Buckets[bucketIndex], key);
		if (alreadyExistingIndex != INDEX_NONE)
		{
			bucket[alreadyExistingIndex] = Pair { key, value };
			return false;
		}

		bucket.Add(Pair { key, value });
		++ValueCount;
		return true;
	}

	bool Add(K&& key, V&& value)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize alreadyExistingIndex = FindPairIndex<Pair, K, K>(Buckets[bucketIndex], key);
		if (alreadyExistingIndex != INDEX_NONE)
		{
			bucket[alreadyExistingIndex] = Pair { Move(key), Move(value) };
			return false;
		}

		bucket.Add({ Move(key), Move(value) });
		++ValueCount;
		return true;
	}

	template<typename InputK>
	void Remove(const InputK& key) requires IsValidHashTableKey<K, InputK>
	{
		const uint64 bucketIndex = Hash<InputK>{}(key) % Buckets.GetLength();

		const usize index = FindPairIndex<Pair, K, InputK>(Buckets[bucketIndex], key);
		CHECK(index != INDEX_NONE);

		Buckets[bucketIndex].Remove(index);
		--ValueCount;
	}

	HashTableIterator<BucketArray, Pair> begin()
	{
		return HashTableIterator<BucketArray, Pair>(Buckets, ValueCount, FindFirstUsedBucket());
	}

	HashTableIterator<BucketArray, Pair> end()
	{
		return HashTableIterator<BucketArray, Pair>(Buckets, 0, 0);
	}

	HashTableIterator<BucketArray, const Pair> begin() const
	{
		return HashTableIterator<BucketArray, const Pair>(Buckets, ValueCount, FindFirstUsedBucket());
	}

	HashTableIterator<BucketArray, const Pair> end() const
	{
		return HashTableIterator<BucketArray, const Pair>(Buckets, 0, 0);
	}

private:
	usize FindFirstUsedBucket() const
	{
		usize index = INDEX_NONE;
		for (usize i = 0; i < Buckets.GetLength(); ++i)
		{
			if (!Buckets[i].IsEmpty())
			{
				index = i;
				break;
			}
		}
		return index;
	}

	BucketArray Buckets;
	usize ValueCount;
	Allocator* Allocator;
};
