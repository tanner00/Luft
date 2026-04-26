#pragma once

#include "Allocator.hpp"
#include "Array.hpp"
#include "Error.hpp"
#include "PlatformCore.hpp"

class StringView
{
public:
	StringView()
		: Buffer(nullptr)
		, Length(0)
	{
	}

	StringView(const char* buffer, usize length)
		: Buffer(buffer)
		, Length(length)
	{
	}

	const char& operator[](usize index) const
	{
		CHECK(index < Length);
		return Buffer[index];
	}

	bool operator==(StringView rhs) const
	{
		return Platform::StringCompare(Buffer, Length, rhs.Buffer, rhs.Length);
	}

	static StringView Empty()
	{
		return StringView();
	}

	const char* GetData() const
	{
		return Buffer;
	}

	usize GetLength() const
	{
		return Length;
	}

	bool IsEmpty() const
	{
		return Length == 0;
	}

	usize Find(char c) const
	{
		for (usize index = 0; index < Length; ++index)
		{
			if (Buffer[index] == c)
			{
				return index;
			}
		}
		return INDEX_NONE;
	}

	usize ReverseFind(char c) const
	{
		for (usize i = Length - 1; i != INDEX_NONE; --i)
		{
			if (Buffer[i] == c)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

private:
	const char* Buffer;
	usize Length;
};

inline StringView operator ""_view(const char* literal, usize length) noexcept
{
	return StringView { literal, length };
}

class String
{
public:
	String()
		: Buffer(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(&GlobalAllocator::Get())
	{
	}

	explicit String(Allocator* allocator)
		: Buffer(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(allocator)
	{
		CHECK(Allocator);
	}

	explicit String(StringView view, Allocator* allocator = &GlobalAllocator::Get())
		: Buffer(nullptr)
		, Length(view.GetLength())
		, Capacity(view.GetLength())
		, Allocator(allocator)
	{
		CHECK(Allocator);

		Buffer = static_cast<char*>(Allocator->Allocate(Length));
		Platform::MemoryCopy(Buffer, view.GetData(), Length);
	}

	explicit String(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: Buffer(nullptr)
		, Length(0)
		, Capacity(capacity)
		, Allocator(allocator)
	{
		CHECK(Allocator);

		Buffer = static_cast<char*>(Allocator->Allocate(Capacity));
	}

	~String()
	{
		if (Allocator)
		{
			Allocator->Deallocate(Buffer, Capacity);
		}
		else
		{
			CHECK(Buffer == nullptr);
		}

		Buffer = nullptr;
		Length = 0;
		Capacity = 0;
	}

	String(const String& copy)
		: Buffer(nullptr)
		, Length(copy.Length)
		, Capacity(copy.Capacity)
		, Allocator(copy.Allocator)
	{
		char* newBuffer = static_cast<char*>(Allocator->Allocate(Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, Length);
		Buffer = newBuffer;
	}

	String& operator=(const String& copy)
	{
		if (&copy == this)
		{
			return *this;
		}

		this->~String();

		Allocator = copy.Allocator;
		char* newBuffer = static_cast<char*>(Allocator->Allocate(copy.Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, copy.Length);
		Buffer = newBuffer;

		Length = copy.Length;
		Capacity = copy.Capacity;

		return *this;
	}

	String(String&& move) noexcept
		: Buffer(move.Buffer)
		, Length(move.Length)
		, Capacity(move.Capacity)
		, Allocator(move.Allocator)
	{
		move.Buffer = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;
	}

	String& operator=(String&& move) noexcept
	{
		if (&move == this)
		{
			return *this;
		}

		this->~String();

		Buffer = move.Buffer;
		Length = move.Length;
		Capacity = move.Capacity;
		Allocator = move.Allocator;

		move.Buffer = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;

		return *this;
	}

	char& operator[](usize index)
	{
		CHECK(index < Length);
		return Buffer[index];
	}

	const char& operator[](usize index) const
	{
		CHECK(index < Length);
		return Buffer[index];
	}

	bool operator==(const String& rhs) const
	{
		return Platform::StringCompare(Buffer, Length, rhs.Buffer, rhs.Length);
	}

	operator StringView() const
	{
		return StringView(Buffer, Length);
	}

	static String Empty(Allocator* allocator = &GlobalAllocator::Get())
	{
		return String(allocator);
	}

	char* GetData() const
	{
		return Buffer;
	}

	usize GetLength() const
	{
		return Length;
	}

	bool IsEmpty() const
	{
		return Length == 0;
	}

	usize Find(char c) const
	{
		for (usize index = 0; index < Length; ++index)
		{
			if (Buffer[index] == c)
			{
				return index;
			}
		}
		return INDEX_NONE;
	}

	usize ReverseFind(char c) const
	{
		for (usize i = Length - 1; i != INDEX_NONE; --i)
		{
			if (Buffer[i] == c)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	void AddUninitialized(usize count)
	{
		if (Length + count > Capacity)
		{
			Grow(Length + count);
		}
		Length += count;
	}

	void Append(char c)
	{
		if (Length == Capacity)
		{
			Grow(Capacity ? (Capacity * 2) : 32);
		}
		Buffer[Length] = c;
		++Length;
	}

	void Append(StringView view)
	{
		const usize newLength = Length + view.GetLength();
		if (newLength > Capacity)
		{
			Grow((Capacity + view.GetLength()) * 2);
		}
		Platform::MemoryCopy(Buffer + Length, view.GetData(), view.GetLength());
		Length = newLength;
	}

	void Reserve(usize capacity)
	{
		CHECK(Buffer == nullptr);
		Capacity = capacity;
		Buffer = static_cast<char*>(Allocator->Allocate(Capacity));
	}

	void Clear()
	{
		Length = 0;
	}

	Array<String> Split(char delimiter, Allocator* allocator = &GlobalAllocator::Get()) const
	{
		CHECK(allocator);

		Array<String> parts(allocator);
		usize start = 0;
		for (usize index = 0; index <= Length; ++index)
		{
			if (index == Length || Buffer[index] == delimiter)
			{
				parts.Add(String(StringView(Buffer + start, index - start), allocator));
				start = index + 1;
			}
		}

		return parts;
	}

private:
	void Grow(usize newCapacity)
	{
		CHECK(newCapacity >= Capacity);

		char* resized = static_cast<char*>(Allocator->Allocate(newCapacity));
		Platform::MemoryCopy(resized, Buffer, Length);
		Allocator->Deallocate(Buffer, Capacity);

		Buffer = resized;
		Capacity = newCapacity;
	}

	char* Buffer;
	usize Length;
	usize Capacity;
	Allocator* Allocator;
};
