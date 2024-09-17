#pragma once

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "Meta.hpp"

template<typename T>
class TrivialArrayIterator
{
public:
	explicit TrivialArrayIterator(T* ptr)
		: Ptr(ptr)
	{
	}

	T& operator*() const { return *Ptr; }
	T* operator->() { return Ptr; }
	TrivialArrayIterator& operator++() { ++Ptr; return *this; }
	friend bool operator==(const TrivialArrayIterator& a, const TrivialArrayIterator& b) { return a.Ptr == b.Ptr; }
	friend bool operator!=(const TrivialArrayIterator& a, const TrivialArrayIterator& b) { return a.Ptr != b.Ptr; }

private:
	T* Ptr;
};

template<typename T> requires IsTriviallyCopyable<T>::Value
class TrivialArray
{
public:
	TrivialArray()
		: Elements(nullptr)
		, Length(0)
		, Capacity(0)
	{
	}

	explicit TrivialArray(usize capacity)
		: Length(0)
		, Capacity(capacity)
	{
		Elements = GlobalAllocate(capacity * sizeof(T));
	}

	~TrivialArray()
	{
		GlobalDeallocate(Elements);

		Elements = nullptr;
		Length = 0;
		Capacity = 0;
	}

	TrivialArray(const TrivialArray& copy)
	{
		Length = copy.Length;
		Capacity = copy.Capacity;

		const usize size = Capacity * sizeof(T);

		T* newElements = static_cast<T*>(GlobalAllocate(size));
		Platform::MemoryCopy(newElements, copy.Elements, Length * sizeof(T));
		Elements = newElements;
	}

	TrivialArray& operator=(const TrivialArray& copy)
	{
		CHECK(&copy != this);

		this->~TrivialArray();

		const usize newSize = copy.Capacity * sizeof(T);
		T* newElements = static_cast<T*>(GlobalAllocate(newSize));
		Platform::MemoryCopy(newElements, copy.Elements, copy.Length * sizeof(T));
		Elements = newElements;

		Length = copy.Length;
		Capacity = copy.Capacity;

		return *this;
	}

	TrivialArray(TrivialArray&& move) noexcept
	{
		Elements = move.Elements;
		Length = move.Length;
		Capacity = move.Capacity;

		move.Elements = nullptr;
		move.Length = 0;
		move.Capacity = 0;
	}

	TrivialArray& operator=(TrivialArray&& move) noexcept
	{
		this->~TrivialArray();

		Elements = move.Elements;
		Length = move.Length;
		Capacity = move.Capacity;

		move.Elements = nullptr;
		move.Length = 0;
		move.Capacity = 0;

		return *this;
	}

	T& operator[](usize index)
	{
		CHECK(index < Length);
		return Elements[index];
	}

	const T& operator[](usize index) const
	{
		CHECK(index < Length);
		return Elements[index];
	}

	T* GetData() const
	{
		CHECK(Elements);
		return Elements;
	}

	usize GetLength() const
	{
		return Length;
	}

	bool IsEmpty() const
	{
		return Length == 0;
	}

	void Add(const T& newElement)
	{
		Emplace(newElement);
	}

	void Add(T&& newElement)
	{
		Emplace(Move(newElement));
	}

	template<typename... Args>
	void Emplace(Args&&... args)
	{
		if (Length == Capacity)
		{
			Grow();
		}
		new (&Elements[Length], ErdeNewMarker {}) T(Forward<Args>(args)...);
		++Length;
	}

	void Reserve(usize capacity)
	{
		CHECK(IsEmpty());
		Capacity = capacity;
		Elements = static_cast<T*>(GlobalAllocate(Capacity * sizeof(T)));
	}

	void Remove(usize index)
	{
		CHECK(index < Length);

		const usize moveLength = Length - index - 1;
		Platform::MemoryMove(Elements + index, Elements + index + 1, moveLength * sizeof(T));

		--Length;
	}

	void Clear()
	{
		Length = 0;
	}

	TrivialArrayIterator<T> begin()
	{
		return TrivialArrayIterator<T>(Elements);
	}

	TrivialArrayIterator<T> end()
	{
		return TrivialArrayIterator<T>(Elements + Length);
	}

private:
	void Grow()
	{
		const usize newCapacity = Capacity ? (Capacity * 2) : 8;

		const usize newSize = newCapacity * sizeof(T);

		T* resized = static_cast<T*>(GlobalAllocate(newSize));
		Platform::MemoryCopy(resized, Elements, Length * sizeof(T));
		GlobalDeallocate(Elements);

		Elements = resized;
		Capacity = newCapacity;
	}

	T* Elements;
	usize Length;
	usize Capacity;
};
