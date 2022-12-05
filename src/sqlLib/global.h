#pragma once

#include <string>
#include <vector>
#include <map>
#include <climits>
#include <mysql.h>

typedef signed char int8;
typedef short int16;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// 복사 생성자와 operator = 을 허용하지 않는 매크로
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete;   \
    TypeName &operator=(const TypeName &) = delete