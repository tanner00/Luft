#pragma once

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "Meta.hpp"

#include <initializer_list>

template<typename T>
class ArrayIterator
{
public:
	explicit ArrayIterator(T* head)
		: Current(head)
	{
	}

	T& operator*() const { return *Current; }
	T* operator->() const { return Current; }
	ArrayIterator& operator++() { ++Current; return *this; }
	bool operator==(const ArrayIterator& rhs) const { return Current == rhs.Current; }

private:
	T* Current;
};

template<typename T>
class ArrayView
{
public:
	ArrayView()
		: Elements(nullptr)
		, Count(0)
	{
	}

	ArrayView(const T* elements, usize count)
		: Elements(elements)
		, Count(count)
	{
	}

	ArrayView(std::initializer_list<T> elements)
		: Elements(elements.begin())
		, Count(elements.size())
	{
	}

	const T& operator[](usize index) const
	{
		CHECK(index < Count);
		return Elements[index];
	}

	static ArrayView Empty()
	{
		return ArrayView();
	}

	T& First()
	{
		CHECK(!IsEmpty());
		return Elements[0];
	}

	const T& First() const
	{
		CHECK(!IsEmpty());
		return Elements[0];
	}

	T& Last()
	{
		CHECK(!IsEmpty());
		return Elements[Count - 1];
	}

	const T& Last() const
	{
		CHECK(!IsEmpty());
		return Elements[Count - 1];
	}

	T* GetData() const
	{
		return Elements;
	}

	usize GetCount() const
	{
		return Count;
	}

	usize GetElementSize() const
	{
		return sizeof(T);
	}

	usize GetDataSize() const
	{
		return Count * GetElementSize();
	}

	bool IsEmpty() const
	{
		return Count == 0;
	}

	ArrayIterator<const T> begin() const
	{
		return ArrayIterator<const T>(Elements);
	}

	ArrayIterator<const T> end() const
	{
		return ArrayIterator<const T>(Elements + Count);
	}

private:
	const T* Elements;
	usize Count;
};

template<typename T>
class Array
{
public:
	Array()
		: Elements(nullptr)
		, Count(0)
		, Capacity(0)
		, Allocator(&GlobalAllocator::Get())
	{
	}

	explicit Array(Allocator* allocator)
		: Elements(nullptr)
		, Count(0)
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit Array(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: Elements(nullptr)
		, Count(0)
		, Capacity(capacity)
		, Allocator(allocator)
	{
		if (Capacity)
		{
			Elements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		}
	}

	Array(std::initializer_list<T> elements)
		: Array(elements.size())
	{
		for (const T& element : elements)
		{
			Add(element);
		}
	}

	~Array()
	{
		if (Allocator)
		{
			if constexpr (!IsTriviallyDestructible<T>::Value)
			{
				for (usize i = 0; i < Count; ++i)
				{
					Elements[i].~T();
				}
			}
			Allocator->Deallocate(Elements, Capacity * sizeof(T));
		}
		else
		{
			CHECK(Elements == nullptr);
		}

		Elements = nullptr;
		Count = 0;
		Capacity = 0;
		Allocator = nullptr;
	}

	Array(const Array& copy)
		: Elements(nullptr)
		, Count(copy.Count)
		, Capacity(copy.Capacity)
		, Allocator(copy.Allocator)
	{
		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, Count * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Count; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T(copy.Elements[i]);
			}
		}
		Elements = newElements;
	}

	Array& operator=(const Array& copy)
	{
		if (&copy == this)
		{
			return *this;
		}

		this->~Array();

		Count = copy.Count;
		Capacity = copy.Capacity;
		Allocator = copy.Allocator;

		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, Count * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Count; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T(copy.Elements[i]);
			}
		}
		Elements = newElements;

		return *this;
	}

	Array(Array&& move) noexcept
		: Elements(move.Elements)
		, Count(move.Count)
		, Capacity(move.Capacity)
		, Allocator(move.Allocator)
	{
		move.Elements = nullptr;
		move.Count = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;
	}

	Array& operator=(Array&& move) noexcept
	{
		if (&move == this)
		{
			return *this;
		}

		this->~Array();

		Elements = move.Elements;
		Count = move.Count;
		Capacity = move.Capacity;
		Allocator = move.Allocator;

		move.Elements = nullptr;
		move.Count = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;

		return *this;
	}

	T& operator[](usize index)
	{
		CHECK(index < Count);
		return Elements[index];
	}

	const T& operator[](usize index) const
	{
		CHECK(index < Count);
		return Elements[index];
	}

	operator ArrayView<T>() const
	{
		return ArrayView<T>(Elements, Count);
	}

	static Array Empty()
	{
		return Array();
	}

	T& First()
	{
		CHECK(!IsEmpty());
		return Elements[0];
	}

	const T& First() const
	{
		CHECK(!IsEmpty());
		return Elements[0];
	}

	T& Last()
	{
		CHECK(!IsEmpty());
		return Elements[Count - 1];
	}

	const T& Last() const
	{
		CHECK(!IsEmpty());
		return Elements[Count - 1];
	}

	T* GetData() const
	{
		return Elements;
	}

	usize GetCount() const
	{
		return Count;
	}

	usize GetElementSize() const
	{
		return sizeof(T);
	}

	usize GetDataSize() const
	{
		return Count * GetElementSize();
	}

	bool IsEmpty() const
	{
		return Count == 0;
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
		if (Count == Capacity)
		{
			const usize doubleCapacity = Capacity * 2;
			Grow(Capacity ? doubleCapacity : 8);
		}
		new (&Elements[Count], LuftNewMarker {}) T(Forward<Args>(args)...);
		++Count;
	}

	void AddUninitialized(usize newCount)
	{
		if (Count + newCount > Capacity)
		{
			Grow(Count + newCount);
		}
		Count += newCount;
	}

	void Reserve(usize totalCapacity)
	{
		CHECK(Elements == nullptr);
		Capacity = totalCapacity;
		Elements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
	}

	void Remove(usize index)
	{
		CHECK(index < Count);

		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			const usize moveCount = Count - index - 1;
			Platform::MemoryMove(Elements + index, Elements + index + 1, moveCount * sizeof(T));
		}
		else
		{
			Elements[index].~T();
			for (usize i = index; i < Count - 1; ++i)
			{
				Elements[i] = MoveIfPossible(Elements[i + 1]);
				Elements[i + 1].~T();
			}
		}

		--Count;
	}

	void Clear()
	{
		if constexpr (!IsTriviallyCopyable<T>::Value)
		{
			for (usize i = 0; i < Count; ++i)
			{
				Elements[i].~T();
			}
		}
		Count = 0;
	}

	T* Surrender()
	{
		T* data = Elements;
		Elements = nullptr;
		Count = 0;
		Capacity = 0;
		return data;
	}

	ArrayIterator<T> begin()
	{
		return ArrayIterator<T>(Elements);
	}

	ArrayIterator<T> end()
	{
		return ArrayIterator<T>(Elements + Count);
	}

	ArrayIterator<const T> begin() const
	{
		return ArrayIterator<const T>(Elements);
	}

	ArrayIterator<const T> end() const
	{
		return ArrayIterator<const T>(Elements + Count);
	}

private:
	void Grow(usize totalCapacity)
	{
		CHECK(totalCapacity >= Capacity);

		const usize totalSize = totalCapacity * sizeof(T);

		T* resized = static_cast<T*>(Allocator->Allocate(totalSize));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(resized, Elements, Count * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Count; ++i)
			{
				new (&resized[i], LuftNewMarker {}) T(MoveIfPossible(Elements[i]));
			}
			for (usize i = 0; i < Count; ++i)
			{
				Elements[i].~T();
			}
		}
		Allocator->Deallocate(Elements, Capacity * sizeof(T));

		Elements = resized;
		Capacity = totalCapacity;
	}

	T* Elements;
	usize Count;
	usize Capacity;
	Allocator* Allocator;
};
