#ifndef __LWBASE_TYPES_H__
#define __LWBASE_TYPES_H__

#include <stddef.h>
#include <inttypes.h>
#include <wchar16.h>

typedef char               LW_CHAR;
typedef wchar16_t          LW_WCHAR;
typedef char *             LW_PSTR;
typedef char const *       LW_PCSTR;
typedef wchar16_t *        LW_PWSTR;
typedef wchar16_t const *  LW_PCWSTR;
typedef int16_t            LW_SHORT;
typedef uint16_t           LW_USHORT;
typedef int32_t            LW_LONG;
typedef uint32_t           LW_ULONG;
typedef int64_t            LW_LONG64;
typedef uint64_t           LW_ULONG64;

#ifndef LW_STRICT_NAMESPACE

#ifndef CHAR_DEFINED
#define CHAR_DEFINED
typedef LW_CHAR CHAR;
#endif

#ifndef WCHAR_DEFINED
#define WCHAR_DEFINED
typedef LW_WCHAR WCHAR;
#endif

#ifndef PSTR_DEFINED
#define PSTR_DEFINED
typedef LW_PSTR PSTR;
#endif

#ifndef PCSTR_DEFINED
#define PCSTR_DEFINED
typedef LW_PCSTR PCSTR;
#endif

#ifndef PWSTR_DEFINED
#define PWSTR_DEFINED
typedef LW_PWSTR PWSTR;
#endif

#ifndef PCWSTR_DEFINED
#define PCWSTR_DEFINED
typedef LW_PCWSTR PCWSTR;
#endif

#ifndef SHORT_DEFINED
#define SHORT_DEFINED
typedef LW_SHORT SHORT;
#endif

#ifndef USHORT_DEFINED
#define USHORT_DEFINED
typedef LW_USHORT USHORT;
#endif

#ifndef LONG_DEFINED
#define LONG_DEFINED
typedef LW_LONG LONG;
#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED
typedef LW_ULONG ULONG;
#endif

#ifndef LONG64_DEFINED
#define LONG64_DEFINED
typedef LW_LONG64 LONG64;
#endif

#ifndef ULONG64_DEFINED
#define ULONG64_DEFINED
typedef LW_ULONG64 ULONG64;
#endif

#endif /* LW_STRICT_NAMESPACE */

#endif
