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
	bool operator==(const ArrayIterator& b) const { return Current == b.Current; }

private:
	T* Current;
};

template<typename T>
class ArrayView
{
public:
	ArrayView()
		: Elements(nullptr)
		, Length(0)
	{
	}

	ArrayView(const T* elements, usize length)
		: Elements(elements)
		, Length(length)
	{
	}

	ArrayView(std::initializer_list<T> elements)
		: Elements(elements.begin())
		, Length(elements.size())
	{
	}

	const T& operator[](usize index) const
	{
		CHECK(index < Length);
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
		return Elements[Length - 1];
	}

	const T& Last() const
	{
		CHECK(!IsEmpty());
		return Elements[Length - 1];
	}

	T* GetData() const
	{
		return Elements;
	}

	usize GetLength() const
	{
		return Length;
	}

	usize GetElementSize() const
	{
		return sizeof(T);
	}

	usize GetDataSize() const
	{
		return Length * GetElementSize();
	}

	bool IsEmpty() const
	{
		return Length == 0;
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
	const T* Elements;
	usize Length;
};

template<typename T>
class Array
{
public:
	Array()
		: Elements(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(&GlobalAllocator::Get())
	{
	}

	explicit Array(Allocator* allocator)
		: Elements(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit Array(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: Elements(nullptr)
		, Length(0)
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
		: Elements(nullptr)
		, Length(copy.Length)
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
				new (&newElements[i], LuftNewMarker {}) T(copy.Elements[i]);
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
		if (&move == this)
		{
			return *this;
		}

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

	operator ArrayView<T>() const
	{
		return ArrayView<T>(Elements, Length);
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
		return Elements[Length - 1];
	}

	const T& Last() const
	{
		CHECK(!IsEmpty());
		return Elements[Length - 1];
	}

	T* GetData() const
	{
		return Elements;
	}

	usize GetLength() const
	{
		return Length;
	}

	usize GetElementSize() const
	{
		return sizeof(T);
	}

	usize GetDataSize() const
	{
		return Length * GetElementSize();
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
			const usize doubleCapacity = Capacity * 2;
			Grow(Capacity ? doubleCapacity : 8);
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
		Length += count;
	}

	void Reserve(usize capacity)
	{
		CHECK(Elements == nullptr);
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
		if constexpr (!IsTriviallyCopyable<T>::Value)
		{
			for (usize i = 0; i < Length; ++i)
			{
				Elements[i].~T();
			}
		}
		Length = 0;
	}

	T* Surrender()
	{
		T* data = Elements;
		Elements = nullptr;
		Length = 0;
		Capacity = 0;
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
		CHECK(newCapacity >= Capacity);

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
				new (&resized[i], LuftNewMarker {}) T(MoveIfPossible(Elements[i]));
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
