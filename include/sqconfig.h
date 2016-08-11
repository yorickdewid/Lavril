#ifdef _LV64

#ifdef _MSC_VER
typedef __int64 SQInteger;
typedef unsigned __int64 SQUnsignedInteger;
typedef unsigned __int64 SQHash; /*should be the same size of a pointer*/
#else
typedef long long SQInteger;
typedef unsigned long long SQUnsignedInteger;
typedef unsigned long long SQHash; /*should be the same size of a pointer*/
#define LVFORMATINT "%lld"
#define LVFORMATINT3 "%03lld"
#endif
typedef int SQInt32;
typedef unsigned int SQUnsignedInteger32;
#else
typedef int SQInteger;
typedef int SQInt32; /*must be 32 bits(also on 64bits processors)*/
typedef unsigned int SQUnsignedInteger32; /*must be 32 bits(also on 64bits processors)*/
typedef unsigned int SQUnsignedInteger;
typedef unsigned int SQHash; /*should be the same size of a pointer*/
#define LVFORMATINT "%d"
#define LVFORMATINT3 "%03d"
#endif

#ifdef USEDOUBLE
typedef double SQFloat;
#else
typedef float SQFloat;
#endif

#if defined(USEDOUBLE) && !defined(_LV64) || !defined(USEDOUBLE) && defined(_LV64)
#ifdef _MSC_VER
typedef __int64 SQRawObjectVal; //must be 64bits
#else
typedef long long SQRawObjectVal; //must be 64bits
#endif
#define SQ_OBJECT_RAWINIT() { _unVal.raw = 0; }
#else
typedef SQUnsignedInteger SQRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
#define SQ_OBJECT_RAWINIT()
#endif

#ifndef SQ_ALIGNMENT // SQ_ALIGNMENT shall be less than or equal to LV_MALLOC alignments, and its value shall be power of 2.
#if defined(USEDOUBLE) || defined(_LV64)
#define SQ_ALIGNMENT 8
#else
#define SQ_ALIGNMENT 4
#endif
#endif

typedef void *LVUserPointer;
typedef SQUnsignedInteger LVBool;
typedef SQInteger LVRESULT;

#ifdef LVUNICODE
#include <wchar.h>
#include <wctype.h>

typedef wchar_t SQChar;

#define scstrcmp    wcscmp
#ifdef _WIN32
#define scsprintf   _snwprintf
#else
#define scsprintf   swprintf
#endif // _WIN32
#define scstrlen    wcslen
#define scstrcpy    wcscpy
#define scstrcat    wcscat
#define scstrtod    wcstod
#ifdef _LV64
#define scstrtol    wcstoll
#else
#define scstrtol    wcstol
#endif // _LV64
#define scstrtoul   wcstoul
#define scvsprintf  vswprintf
#define scstrstr    wcsstr
#define scprintf    wprintf

#ifdef _WIN32
#define WCHAR_SIZE 2
#define WCHAR_SHIFT_MUL 1
#define MAX_CHAR 0xFFFF
#else
#define WCHAR_SIZE 4
#define WCHAR_SHIFT_MUL 2
#define MAX_CHAR 0xFFFFFFFF
#endif // _WIN32

#define _LC(a) L##a

#define scisspace   iswspace
#define scisdigit   iswdigit
#define scisprint   iswprint
#define scisxdigit  iswxdigit
#define scisalpha   iswalpha
#define sciscntrl   iswcntrl
#define scisalnum   iswalnum

#define sq_rsl(l) ((l)<<WCHAR_SHIFT_MUL)

#else
typedef char SQChar;
#define _LC(a) a
#define scstrcmp    strcmp
#ifdef _MSC_VER
#define scsprintf   _snprintf
#else
#define scsprintf   snprintf
#endif // _MSC_VER
#define scstrlen    strlen
#define scstrcpy    strcpy
#define scstrcat    strcat
#define scstrtod    strtod
#ifdef _LV64
#ifdef _MSC_VER
#define scstrtol    _strtoi64
#else
#define scstrtol    strtoll
#endif // _MSC_VER
#else
#define scstrtol    strtol
#endif // _LV64
#define scstrtoul   strtoul
#define scvsprintf  vsnprintf
#define scstrstr    strstr
#define scisspace   isspace
#define scisdigit   isdigit
#define scisprint   isprint
#define scisxdigit  isxdigit
#define sciscntrl   iscntrl
#define scisalpha   isalpha
#define scisalnum   isalnum
#define scprintf    printf
#define MAX_CHAR 0xFF

#define sq_rsl(l) (l)

#endif // LVUNICODE

#ifdef _LV64
#define _PRINT_INT_PREC _LC("ll")
#define _PRINT_INT_FMT _LC("%lld")
#else
#define _PRINT_INT_FMT _LC("%d")
#endif // _LV64
