#if !defined(win32_types_h)
#define win32_types_h

//  Fixed-length type
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;

#ifndef uint8
typedef unsigned char uint8;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uint32
typedef unsigned long uint32;
#endif

#ifndef int8
typedef signed char int8;
#endif
#ifndef int16
typedef signed short int16;
#endif
#ifndef int32
typedef signed long int32;
#endif

#endif // win32_types_h
