/*********************************************************************++*//**
 * \brief Utility definitions and macros.
 *
 * \author Jesús Alonso (doragasu)
 * \date 2017
 *
 * \defgroup Utility util
 * \{
 ****************************************************************************/
#ifndef _UTIL_H_
#define _UTIL_H_

/// Try to detect if compiler is running under Windows
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define __OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef TRUE
/// For evaluation to TRUE of statements
#define TRUE 1
#endif
#ifndef FALSE
/// For evaluation to FALSE of statements
#define FALSE 0
#endif

#ifndef MAX
/// Obtains the maximum value between two numbers.
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif
#ifndef MIN
/// Obtains the minimum value between two numbers.
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif

/// printf-like macro that writes on stderr instead of stdout
#define PrintErr(...)	do{fprintf(stderr, __VA_ARGS__);}while(0)

/// Delay ms function, compatible with both Windows and Unix
#ifdef __OS_WIN
#define DelayMs(ms) Sleep(ms)
#else
#define DelayMs(ms) usleep((ms)*1000)
#endif

/// 16-bit byte swap macro
#define ByteSwapWord(word)	do{(word) = ((word)>>8) | ((word)<<8);}while(0)

#endif //_UTIL_H_

/** \} */

