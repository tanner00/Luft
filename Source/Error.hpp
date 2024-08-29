#pragma once

#include "Platform.hpp"

#if DEBUG
#define CHECK(condition) do { if (!(condition)) { BREAK_IN_DEBUGGER(); } } while (0)
#define VERIFY(condition, errorMessage) CHECK((condition))
#else
#define CHECK(condition) (void)(condition)
#define VERIFY(condition, errorMessage) do { if (!(condition)) { Platform::FatalError((errorMessage)); } } while (0)
#endif
