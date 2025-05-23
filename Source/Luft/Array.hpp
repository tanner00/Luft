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
class ArrayView
{
public:
	ArrayView()
		: Elements(nullptr)
		, Length(0)
	{
	}

	ArrayView(T* elements, usize length)
		: Elements(elements)
		, Length(length)
	{
	}

	ArrayView(std::initializer_list<T> elements) requires IsConst<T>::Value
		: Elements(elements.begin())
		, Length(elements.size())
	{
	}

	static ArrayView Empty()
	{
		return ArrayView {};
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

protected:
	T* Elements;
	usize Length;
};

template<typename T>
class Array final : public ArrayView<T>
{
public:
	explicit Array(Allocator* allocator = &GlobalAllocator::Get())
		: ArrayView<T>()
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit Array(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: ArrayView<T>()
		, Capacity(capacity)
		, Allocator(allocator)
	{
		this->Elements = Capacity ? static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T))) : nullptr;
	}

	~Array()
	{
		if (Allocator)
		{
			if constexpr (!IsTriviallyDestructible<T>::Value)
			{
				for (usize i = 0; i < this->Length; ++i)
				{
					this->Elements[i].~T();
				}
			}
			Allocator->Deallocate(this->Elements, Capacity * sizeof(T));
		}
		else
		{
			CHECK(this->Elements == nullptr);
		}

		this->Elements = nullptr;
		this->Length = 0;
		Capacity = 0;
		Allocator = nullptr;
	}

	Array(const Array& copy)
		: ArrayView<T>(nullptr, copy.Length)
		, Capacity(copy.Capacity)
		, Allocator(copy.Allocator)
	{
		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, this->Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < this->Length; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T { copy.Elements[i] };
			}
		}
		this->Elements = newElements;
	}

	Array& operator=(const Array& copy)
	{
		if (&copy == this)
		{
			return *this;
		}

		this->~Array();

		this->Length = copy.Length;
		Capacity = copy.Capacity;
		Allocator = copy.Allocator;

		T* newElements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(newElements, copy.Elements, this->Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < this->Length; ++i)
			{
				new (&newElements[i], LuftNewMarker {}) T { copy.Elements[i] };
			}
		}
		this->Elements = newElements;

		return *this;
	}

	Array(Array&& move) noexcept
		: ArrayView<T>(move.Elements, move.Length)
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

		this->Elements = move.Elements;
		this->Length = move.Length;
		Capacity = move.Capacity;
		Allocator = move.Allocator;

		move.Elements = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;

		return *this;
	}

	const ArrayView<T>& AsView() const
	{
		return *this;
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
		if (this->Length == Capacity)
		{
			const usize doubleCapacity = Capacity * 2;
			Grow(Capacity ? doubleCapacity : 8);
		}
		new (&this->Elements[this->Length], LuftNewMarker {}) T(Forward<Args>(args)...);
		++this->Length;
	}

	void AddUninitialized(usize count)
	{
		if (this->Length + count > Capacity)
		{
			const usize doubleCapacity = Capacity * 2;
			Grow((this->Length + count > doubleCapacity) ? (this->Length + count) : doubleCapacity);
		}
		this->Length += count;
	}

	void GrowToLengthUninitialized(usize count)
	{
		if (this->Length + count > Capacity)
		{
			Grow(this->Length + count);
		}
		this->Length = count;
	}

	void Reserve(usize capacity)
	{
		CHECK(this->Elements == nullptr);
		Capacity = capacity;
		this->Elements = static_cast<T*>(Allocator->Allocate(Capacity * sizeof(T)));
	}

	void Remove(usize index)
	{
		CHECK(index < this->Length);

		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			const usize moveLength = this->Length - index - 1;
			Platform::MemoryMove(this->Elements + index, this->Elements + index + 1, moveLength * sizeof(T));
		}
		else
		{
			this->Elements[index].~T();
			for (usize i = index; i < this->Length - 1; ++i)
			{
				this->Elements[i] = MoveIfPossible(this->Elements[i + 1]);
				this->Elements[i + 1].~T();
			}
		}

		--this->Length;
	}

	void Clear()
	{
		if constexpr (!IsTriviallyCopyable<T>::Value)
		{
			for (usize i = 0; i < this->Length; ++i)
			{
				this->Elements[i].~T();
			}
		}
		this->Length = 0;
	}

	T* Surrender()
	{
		T* data = this->Elements;
		this->Elements = nullptr;
		this->Length = 0;
		Capacity = 0;
		return data;
	}

private:
	void Grow(usize newCapacity)
	{
		const usize newSize = newCapacity * sizeof(T);

		T* resized = static_cast<T*>(Allocator->Allocate(newSize));
		if constexpr (IsTriviallyCopyable<T>::Value)
		{
			Platform::MemoryCopy(resized, this->Elements, this->Length * sizeof(T));
		}
		else
		{
			for (usize i = 0; i < this->Length; ++i)
			{
				new (&resized[i], LuftNewMarker {}) T { MoveIfPossible(this->Elements[i]) };
			}
			for (usize i = 0; i < this->Length; ++i)
			{
				this->Elements[i].~T();
			}
		}
		Allocator->Deallocate(this->Elements, Capacity * sizeof(T));

		this->Elements = resized;
		Capacity = newCapacity;
	}

	usize Capacity;
	Allocator* Allocator;
};
