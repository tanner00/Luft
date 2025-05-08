#pragma once

#include "Allocator.hpp"
#include "Error.hpp"
#include "Base.hpp"
#include "Platform.hpp"

class StringView
{
public:
	StringView()
		: Buffer(nullptr)
		, Length(0)
	{
	}

	StringView(char* buffer, usize length)
		: Buffer(buffer)
		, Length(length)
	{
	}

	static StringView Empty()
	{
		return StringView {};
	}

	char* GetData() const
	{
		CHECK(Buffer);
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
		for (usize i = 0; i < Length; ++i)
		{
			if (Buffer[i] == c)
			{
				return i;
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

	bool operator==(const StringView& b) const
	{
		if (Length != b.Length)
		{
			return false;
		}
		return Platform::StringCompare(Buffer, Length, b.Buffer, b.Length);
	}

	bool operator!=(const StringView& b) const
	{
		return !(*this == b);
	}

protected:
	char* Buffer;
	usize Length;
};

inline StringView operator ""_view(const char* literal, usize length) noexcept
{
	return StringView { const_cast<char*>(literal), length };
}

class String final : public StringView
{
public:
	explicit String(Allocator* allocator = &GlobalAllocator::Get())
		: StringView(nullptr, 0)
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit String(StringView view, Allocator* allocator = &GlobalAllocator::Get())
		: StringView(nullptr, view.GetLength())
		, Capacity(view.GetLength())
		, Allocator(allocator)
	{
		Buffer = static_cast<char*>(Allocator->Allocate(Capacity));
		Platform::MemoryCopy(Buffer, view.GetData(), Capacity);
	}

	explicit String(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: StringView(nullptr, 0)
		, Capacity(capacity)
		, Allocator(allocator)
	{
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
		: StringView(copy)
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
		: StringView(move)
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

	const StringView& AsView() const
	{
		return *this;
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

	void Append(StringView view, usize length)
	{
		const usize newLength = Length + length;
		if (newLength > Capacity)
		{
			Grow((Capacity + length) * 2);
		}
		Platform::MemoryCopy(Buffer + Length, view.GetData(), length);
		Length = newLength;
	}

	void Append(StringView view)
	{
		Append(view, view.GetLength());
	}

	void Reserve(usize capacity)
	{
		CHECK(IsEmpty());
		Capacity = capacity;
		Buffer = static_cast<char*>(Allocator->Allocate(Capacity));
	}

	void Clear()
	{
		Length = 0;
	}

private:
	void Grow(usize newCapacity)
	{
		char* resized = static_cast<char*>(Allocator->Allocate(newCapacity));
		Platform::MemoryCopy(resized, Buffer, Length);
		Allocator->Deallocate(Buffer, Capacity);

		Buffer = resized;
		Capacity = newCapacity;
	}

	usize Capacity;
	Allocator* Allocator;
};
