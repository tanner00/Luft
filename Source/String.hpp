#pragma once

#include "Allocator.hpp"
#include "Base.hpp"
#include "Platform.hpp"

class StringView
{
public:
	const uint8& operator[](usize index) const
	{
		CHECK(index < Length);
		return Buffer[index];
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
	String()
		: Buffer(nullptr)
		, Length(0)
		, Capacity(0)
	{
	}

	String(StringView view)
		: Length(view.Length)
		, Capacity(view.Length)
	{
		Buffer = static_cast<uint8*>(GlobalAllocate(Capacity));
		Platform::MemoryCopy(Buffer, view.Buffer, Capacity);
	}

	String(const uint8* data, usize length)
		: String(StringView { data, length })
	{
	}

	explicit String(usize capacity)
	{
		Length = 0;
		Capacity = capacity;
		Buffer = static_cast<uint8*>(GlobalAllocate(Capacity));
	}

	~String()
	{
		GlobalDeallocate(Buffer);

		Buffer = nullptr;
		Length = 0;
		Capacity = 0;
	}

	String(const String& copy)
	{
		Length = copy.Length;
		Capacity = copy.Capacity;

		uint8* newBuffer = static_cast<uint8*>(GlobalAllocate(Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, Length);
		Buffer = newBuffer;
	}

	String& operator=(const String& copy)
	{
		CHECK(&copy != this);

		this->~String();

		uint8* newBuffer = static_cast<uint8*>(GlobalAllocate(copy.Capacity));
		Platform::MemoryCopy(newBuffer, copy.Buffer, copy.Capacity);
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

		move.Buffer = nullptr;
		move.Length = 0;
		move.Capacity = 0;
	}

	String& operator=(String&& move) noexcept
	{
		this->~String();

		Buffer = move.Buffer;
		Length = move.Length;
		Capacity = move.Capacity;

		move.Buffer = nullptr;
		move.Length = 0;
		move.Capacity = 0;

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
		const usize newLength = Length + 1;
		if (newLength >= Capacity)
		{
			Grow(newLength);
		}
		Buffer[Length] = c;
		Length = newLength;
	}

	void Append(StringView view)
	{
		const usize newLength = Length + view.Length;
		if (newLength >= Capacity)
		{
			Grow(newLength);
		}
		Platform::MemoryCopy(Buffer + Length, view.Buffer, view.Length);
		Length = newLength;
	}

	void Reserve(usize capacity)
	{
		CHECK(IsEmpty());
		Capacity = capacity;
		Buffer = static_cast<uint8*>(GlobalAllocate(Capacity));
	}

	void Clear()
	{
		Length = 0;
	}

	StringView AsView() const
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
	void Grow(usize atLeast)
	{
		const usize growth = Capacity ? (Capacity * 2) : 32;
		const usize newCapacity = (growth >= atLeast) ? growth : atLeast;

		uint8* resized = static_cast<uint8*>(GlobalAllocate(newCapacity));
		Platform::MemoryCopy(resized, Buffer, Length);
		GlobalDeallocate(Buffer);

		Buffer = resized;
		Capacity = newCapacity;
	}

	uint8* Buffer;
	usize Length;
	usize Capacity;
};
