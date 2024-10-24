#pragma once

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "Meta.hpp"

template<typename T>
class ArrayIterator
{
public:
	explicit ArrayIterator(T* ptr)
		: Ptr(ptr)
	{
	}

	T& operator*() const { return *Ptr; }
	T* operator->() const { return Ptr; }
	ArrayIterator& operator++() { ++Ptr; return *this; }
	bool operator==(const ArrayIterator& b) { return Ptr == b.Ptr; }
	bool operator!=(const ArrayIterator& b) { return Ptr != b.Ptr; }

private:
	T* Ptr;
};

template<typename T>
class Array
{
public:
	explicit Array(Allocator* allocator = &GlobalAllocator::Get())
		: Elements(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit Array(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: Length(0)
		, Capacity(capacity)
		, Allocator(allocator)
	{
		Elements = Capacity ? static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T))) : nullptr;
	}

	~Array()
	{
		if (Allocator)
		{
			if constexpr (!IsTriviallyDestructible<T>::Value)
			{
				for (usize i = 0; i < Length; ++i)
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
		Length = 0;
		Capacity = 0;
		Allocator = nullptr;
	}

	Array(const Array& copy)
		: Length(copy.Length)
		, Capacity(copy.Capacity)
		, Allocator(copy.Allocator)
	{
		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Length; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T { copy.Elements[i] };
			}
		}
		Elements = newElements;
	}

	Array& operator=(const Array& copy)
	{
		CHECK(&copy != this);

		this->~Array();

		Length = copy.Length;
		Capacity = copy.Capacity;
		Allocator = copy.Allocator;

		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Length; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T { copy.Elements[i] };
			}
		}
		Elements = newElements;

		return *this;
	}

	Array(Array&& move) noexcept
		: Elements(move.Elements)
		, Length(move.Length)
		, Capacity(move.Capacity)
		, Allocator(move.Allocator)
	{
		move.Elements = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;
	}

	Array& operator=(Array&& move) noexcept
	{
		this->~Array();

		Elements = move.Elements;
		Length = move.Length;
		Capacity = move.Capacity;
		Allocator = move.Allocator;

		move.Elements = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;

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
			Grow(Capacity ? (Capacity * 2) : 8);
		}
		new (&Elements[Length], LuftNewMarker {}) T(Forward<Args>(args)...);
		++Length;
	}

	void AddUninitialized(usize count)
	{
		if (Length + count > Capacity)
		{
			Grow(Length + count);
		}
		Length = count;
	}

	void Reserve(usize capacity)
	{
		CHECK(IsEmpty());
		Capacity = capacity;
		Elements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
	}

	void Remove(usize index)
	{
		CHECK(index < Length);

		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			const usize moveLength = Length - index - 1;
			Platform::MemoryMove(Elements + index, Elements + index + 1, moveLength * sizeof(T));
		}
		else
		{
			Elements[index].~T();
			for (usize i = index; i < Length - 1; ++i)
			{
				Elements[i] = MoveIfPossible(Elements[i + 1]);
				Elements[i + 1].~T();
			}
		}

		--Length;
	}

	void Clear()
	{
		Length = 0;
	}

	T* Surrender()
	{
		T* data = Elements;
		Elements = nullptr;
		Length = 0;
		Capacity = 0;
		Allocator = nullptr;
		return data;
	}

	ArrayIterator<T> begin()
	{
		return ArrayIterator<T>(Elements);
	}

	ArrayIterator<T> end()
	{
		return ArrayIterator<T>(Elements + Length);
	}

	ArrayIterator<const T> begin() const
	{
		return ArrayIterator<const T>(Elements);
	}

	ArrayIterator<const T> end() const
	{
		return ArrayIterator<const T>(Elements + Length);
	}

private:
	void Grow(usize newCapacity)
	{
		const usize newSize = newCapacity * sizeof(T);

		T* resized = static_cast<T*>(Allocator->Allocate(newSize));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(resized, Elements, Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < Length; ++i)
			{
				new (&resized[i], LuftNewMarker {}) T { MoveIfPossible(Elements[i]) };
			}
			for (usize i = 0; i < Length; ++i)
			{
				Elements[i].~T();
			}
		}
		Allocator->Deallocate(Elements, Capacity * sizeof(T));

		Elements = resized;
		Capacity = newCapacity;
	}

	T* Elements;
	usize Length;
	usize Capacity;
	Allocator* Allocator;
};
