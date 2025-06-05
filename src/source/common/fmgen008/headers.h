#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

#define STRICT
#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

#endif	// WIN_HEADERS_H
