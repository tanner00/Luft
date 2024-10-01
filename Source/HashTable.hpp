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

	explicit HashTable(usize bucketCount, Allocator* allocator = &GlobalAllocator::Get())
		: ValueCount(0)
		, Allocator(allocator)
	{
		CHECK(bucketCount > 0);
		new (&Buckets, LuftNewMarker {}) Array<Array<Pair>> (bucketCount, Allocator);
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
		new (&Buckets, LuftNewMarker {}) Array<Array<Pair>> (copy.Buckets.GetLength(), Allocator);
		for (Array<Pair>& bucket : copy.Buckets)
		{
			bucket = copy.Buckets;
		}
	}

	HashTable& operator=(const HashTable& copy)
	{
		CHECK(&copy != this);

		this->~HashTable();

		ValueCount = copy.ValueCount;
		Allocator = copy.Allocator;

		new (&Buckets, LuftNewMarker {}) Array<Array<Pair>> (copy.Buckets.GetLength(), Allocator);
		for (Array<Pair>& bucket : copy.Buckets)
		{
			bucket = copy.Buckets;
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

	V& operator[](const K& key)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex(bucket, key);
		CHECK(index != INDEX_NONE);
		return bucket[index].Value;
	}

	const V& operator[](const K& key) const
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		const Array<Pair>& bucket = Buckets[bucketIndex];

		const usize index = FindPairIndex(bucket, key);
		CHECK(index != INDEX_NONE);
		return bucket[index].Value;
	}

	usize GetCount() const
	{
		return ValueCount;
	}

	bool IsEmpty() const
	{
		return ValueCount == 0;
	}

	bool Contains(const K& key) const
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		return FindPairIndex(Buckets[bucketIndex], key) != INDEX_NONE;
	}

	bool Add(const K& key, const V& value)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();
		Array<Pair>& bucket = Buckets[bucketIndex];

		const usize alreadyExistingIndex = FindPairIndex(Buckets[bucketIndex], key);
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

		const usize alreadyExistingIndex = FindPairIndex(Buckets[bucketIndex], key);
		if (alreadyExistingIndex != INDEX_NONE)
		{
			bucket[alreadyExistingIndex] = Pair { Move(key), Move(value) };
			return false;
		}

		bucket.Add({ Move(key), Move(value) });
		++ValueCount;
		return true;
	}

	void Remove(const K& key)
	{
		const uint64 bucketIndex = Hash<K>{}(key) % Buckets.GetLength();

		const usize index = FindPairIndex(Buckets[bucketIndex], key);
		CHECK(index != INDEX_NONE);

		Buckets[bucketIndex].Remove(index);
		--ValueCount;
	}

	using BucketArray = Array<Array<Pair>>;

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
	usize FindPairIndex(const Array<Pair>& bucket, const K& key) const
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
