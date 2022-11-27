#pragma once

#include "middleware/logger.h"
#include <atomic>
#include <csignal>
#include <thread>
#include <memory>

#include <cstddef>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define VERIFY(x)                                                                        \
    if (!(x))                                                                            \
    {                                                                                    \
        LogError("Verification failed: %s, file %s, line %i", #x, __FILE__, __LINE__);   \
        std::abort();                                                                    \
    }

#ifdef _DEBUG
#define ASSERT(x)                                                                        \
    if (!(x))                                                                            \
    {                                                                                    \
        LogError("Assertion failed: %s, file %s, line %i", #x, __FILE__, __LINE__);      \
        std::abort();                                                                    \
    }
#else
#define ASSERT(x)
#endif

typedef signed char int8;
typedef short int16;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// 복사 생성자와 operator = 을 허용하지 않는 매크로
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                               \
    TypeName(const TypeName&) = delete;                                                  \
    TypeName& operator=(const TypeName&) = delete

using ContiguousByteContainer = std::basic_string<std::byte>;
using ContiguousByteViewContainer = std::basic_string_view<std::byte>;