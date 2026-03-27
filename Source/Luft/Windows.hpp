#pragma once

#include "Allocator.hpp"
#include "Array.hpp"
#include "String.hpp"

namespace Windows
{

Array<wchar_t> UTF8ToWide(StringView utf8, Allocator* allocator = &GlobalAllocator::Get());
String WideToUTF8(const wchar_t* wide, Allocator* allocator = &GlobalAllocator::Get());

}
