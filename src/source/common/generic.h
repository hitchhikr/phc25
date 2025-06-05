//============================================================================
//  generic.h Type definition header file PS2EE/PS2IOP/PS1/GBA/PokeStation/Win32 version
//----------------------------------------------------------------------------
//													Programmed by Keiichi Tago
//----------------------------------------------------------------------------
//* Little endian only (x86, EE, IOP, ARM, etc.)
//   PokeStation is unfinished
//----------------------------------------------------------------------------
// 2004.06.16 Ver.1.08 Changed from define to typrdef. Supports NDS.
// 2004.02.23 Ver.1.07 The bool definition has been removed.
// 2003.07.29 Ver.1.06 Removed unnecessary constants. Cleaned up UxxDATA.
// 2003.06.26 Ver.1.05 The -DGBA option is required for GBA compilation.
// 2003.04.24 Ver.1.04 NULL is defined.
// 2003.04.08 Ver.1.03 Now compatible with GBA.
// 2003.02.20 Ver.1.02 bool definition.
// 2003.02.20 Ver.1.01 Supports IOP. Minor optimizations.
// 2003.02.14 Ver.1.00 Changed the file name of ktDefine.h. Added support for PS2EE.
//============================================================================
#ifndef GENERIC_INCLUDE
#define GENERIC_INCLUDE

// -------------------- Header file registration
#if defined _WIN32
	#include <windows.h>
#elif defined R5900
	#include <eetypes.h>
#elif defined R3000
	#include <sys/types.h>
#elif defined SDK_ARM9
	#include <nitro.h>
#endif

// -------------------- Model-specific definition
#if defined _WIN32
// Win32 type definition
typedef char				S8;
typedef short				S16;
typedef long				S32;
typedef __int64				S64;
typedef unsigned char		U8;
typedef unsigned short		U16;
typedef unsigned long		U32;
typedef unsigned __int64	U64;
#elif defined R5900
// PS2EE type definition
typedef char				S8;
typedef	short				S16;
typedef	int					S32;
typedef	long				S64;
typedef	long128				S128;
typedef unsigned char		U8;
typedef	unsigned short		U16;
typedef	unsigned int		U32;
typedef	unsigned long		U64;
typedef	u_long128			U128;
#elif defined R3000
// PS1/PS2IOP type definition
typedef	char				S8;
typedef	short				S16;
typedef	int					S32;
typedef	long				S64;
typedef	unsigned char		S8;
typedef	unsigned short		S16;
typedef	unsigned int		S32;
typedef	unsigned long		S64;
#elif defined GBA
// GBA type definition
typedef	char				S8;
typedef	short				S16;
typedef	long				S32;
typedef	unsigned char		U8;
typedef	unsigned short		U16;
typedef	unsigned long		U32;
#elif defined SDK_ARM9
// NDS type definition
typedef	signed char				S8;
typedef	signed short int		S16;
typedef	signed long				S32;
typedef	signed long long int	S64;
typedef	unsigned char			U8;
typedef	unsigned short int		U16;
typedef	unsigned long			U32;
typedef	unsigned long long int	U64;
#endif

// Match Windows
#ifndef _WIN32
typedef	U8	BYTE;
typedef U8	*LPBYTE;
typedef U16	WORD;
typedef U16	*LPWORD;
typedef U32	DWORD;
typedef U32	*LPDWORD;
#endif

// BOOL definition
#ifndef BOOL
#define BOOL  int
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

// NULL definition
#ifndef NULL
#define NULL 0
#endif

#endif
