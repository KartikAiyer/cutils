/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2019> <Kartik Aiyer>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CUTILS_TYPES_H
#define CUTILS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define GetArraySize(x)                   ((sizeof((x)))/(sizeof((x)[0])))

#ifndef MIN
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef CUTILS_ASSERT
#define CUTILS_ASSERT(x) {\
  if(!(x)) {\
    *((volatile char *)0) = 1;\
  }\
}
#endif

// expansion macro for enum value definition
#define ENUM_VALUE( name, assign ) name assign,

// expansion macro for enum to string conversion
#define ENUM_CASE( name, assign ) case name: return #name;

#define ENUM_STRCMP( name, assign ) if( !strcmp( str, #name ) ) return name;

// declare the access function and enum values
#define DECLARE_ENUM( EnumType, ENUM_DEF ) \
  typedef enum _##EnumType { \
    ENUM_DEF( ENUM_VALUE ) \
  }EnumType;\
  static inline const char* get_##EnumType##_string( EnumType blah )\
  {\
     switch( blah ) {\
       ENUM_DEF(ENUM_CASE)\
       default: return "";\
     }\
  }\
  static inline EnumType get_##EnumType##_value( const char* str )\
  {\
     ENUM_DEF(ENUM_STRCMP)\
     return (EnumType)0;\
  }

#define DECLARE_ENUM_NONINLINE( EnumType, ENUM_DEF )\
  typedef enum _##EnumType { \
    ENUM_DEF( ENUM_VALUE ) \
  }EnumType;\
  const char* get_##EnumType##_string( EnumType blah );\
  EnumType get_##EnumType##_value( const char* str );

#define DEFINE_ENUM_STRINGIFY( EnumType, ENUM_DEF )\
  const char* get_##EnumType##_string( EnumType blah )\
  {\
     switch( blah ) {\
       ENUM_DEF(ENUM_CASE)\
       default: return "";\
     }\
  }\
  EnumType get_##EnumType##_value( const char* str )\
  {\
     ENUM_DEF(ENUM_STRCMP)\
     return (EnumType)0;\
  }

// Other system wide constants and macros
#define LOG_MAX_LINE_LENGTH             (512)

#ifdef __cplusplus
};
#endif
#endif //CUTILS_TYPES_H
