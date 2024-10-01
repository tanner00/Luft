#pragma once

#include "Allocator.hpp"
#include "Error.hpp"
#include "Base.hpp"
#include "Platform.hpp"

class StringView
{
public:
	uint8 operator[](usize index) const
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
		return Platform::StringCompare(reinterpret_cast<const char*>(Buffer), Length, reinterpret_cast<const char*>(b.Buffer), b.Length);
	}

	bool operator!=(const StringView& b) const
	{
		return !(*this == b);
	}

	const uint8* Buffer;
	usize Length;
};

inline StringView operator ""_view(const char* literal, usize length) noexcept
{
	return StringView { reinterpret_cast<const uint8*>(literal), length };
}

class String
{
public:
	explicit String(Allocator* allocator = &GlobalAllocator::Get())
		: Buffer(nullptr)
		, Length(0)
		, Capacity(0)
		, Allocator(allocator)
	{
	}

	explicit String(StringView view, Allocator* allocator = &GlobalAllocator::Get())
		: Length(view.Length)
		, Capacity(view.Length)
		, Allocator(allocator)
	{
		Buffer = static_cast<uint8*>(Allocator->Allocate(Capacity));
		Platform::MemoryCopy(Buffer, view.Buffer, Capacity);
	}

	String(const uint8* data, usize length, Allocator* allocator = &GlobalAllocator::Get())
		: String(StringView { data, length }, allocator)
	{
	}

	explicit String(usize capacity, Allocator* allocator = &GlobalAllocator::Get())
		: Length(0)
		, Capacity(capacity)
		, Allocator(allocator)
	{
		Buffer = static_cast<uint8*>(Allocator->Allocate(Capacity));
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
	{
		Length = copy.Length;
		Capacity = copy.Capacity;
		Allocator = copy.Allocator;

		uint8* newBuffer = static_cast<uint8*>(Allocator->Allocate(Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, Length);
		Buffer = newBuffer;
	}

	String& operator=(const String& copy)
	{
		CHECK(&copy != this);

		this->~String();

		Allocator = copy.Allocator;
		uint8* newBuffer = static_cast<uint8*>(Allocator->Allocate(copy.Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, copy.Length);
		Buffer = newBuffer;

		Length = copy.Length;
		Capacity = copy.Capacity;

		return *this;
	}

	String(String&& move) noexcept
	{
		Buffer = move.Buffer;
		Length = move.Length;
		Capacity = move.Capacity;
		Allocator = move.Allocator;

		move.Buffer = nullptr;
		move.Length = 0;
		move.Capacity = 0;
		move.Allocator = nullptr;
	}

	String& operator=(String&& move) noexcept
	{
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

	uint8& operator[](usize index)
	{
		CHECK(index < Length);
		return Buffer[index];
	}

	const uint8& operator[](usize index) const
	{
		CHECK(index < Length);
		return Buffer[index];
	}

	uint8* GetData() const
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

	void Append(uint8 c)
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
		const usize newLength = Length + view.Length;
		if (newLength > Capacity)
		{
			Grow((Capacity + view.Length) * 2);
		}
		Platform::MemoryCopy(Buffer + Length, view.Buffer, view.Length);
		Length = newLength;
	}

	void Reserve(usize capacity)
	{
		CHECK(IsEmpty());
		Capacity = capacity;
		Buffer = static_cast<uint8*>(Allocator->Allocate(Capacity));
	}

	void Clear()
	{
		Length = 0;
	}

	operator StringView() const
	{
		return StringView { Buffer, Length };
	}

	bool operator==(const String& b) const
	{
		if (Length != b.Length)
		{
			return false;
		}
		return Platform::StringCompare(reinterpret_cast<const char*>(Buffer), Length, reinterpret_cast<const char*>(b.Buffer), b.Length);
	}

	bool operator!=(const String& b) const
	{
		return !(*this == b);
	}

	bool operator==(const StringView& b) const
	{
		if (Length != b.Length)
		{
			return false;
		}
		return Platform::StringCompare(reinterpret_cast<const char*>(Buffer), Length, reinterpret_cast<const char*>(b.Buffer), b.Length);
	}

	bool operator!=(const StringView& b) const
	{
		return !(*this == b);
	}

private:
	void Grow(usize newCapacity)
	{
		uint8* resized = static_cast<uint8*>(Allocator->Allocate(newCapacity));
		Platform::MemoryCopy(resized, Buffer, Length);
		Allocator->Deallocate(Buffer, Capacity);

		Buffer = resized;
		Capacity = newCapacity;
	}

	uint8* Buffer;
	usize Length;
	usize Capacity;
	Allocator* Allocator;
};
