//============================================================================
//  debug.h "Debug control header file" PS2/Win32/GBA/NDS version
//----------------------------------------------------------------------------
//													Programmed by Tago
//----------------------------------------------------------------------------
// 2004.06.29 Ver.1.12 Fixed a warning message on NDS.
// 2004.06.14 Ver.1.11 Added DEBUGFILECLEAR() which changed the output file name to report.txt.
// 2004.05.28 Ver.1.10 The following modifications were model dependent and have been corrected.
// 2004.05.26 Ver.1.09 DEBUG.TXT has been fixed to be in the same location as the executable file.
// 2004.02.23 Ver.1.08 The size of malloc has been changed from int to size_t.
// 2004.02.18 Ver.1.07 Fixed a strange DirectX error string.
// 2003.07.11 Ver.1.06 1.05 could not be compiled with VC.NET, so this has been fixed.
// 2003.07.10 Ver.1.05 1.04 could not be compiled on GBA, so this has been fixed.
// 2003.06.26 Ver.1.04 Fixed a warning that occurred when compiling with VC++.
// 2003.05.28 Ver.1.03 new/delete are not overloaded by default.
//                     (The usage of new/delete is incorrect. They should not be overloaded.)
//                     To compile .GBA, you must use the -DGBA option.
// 2003.05.06 Ver.1.02 Support for writing files via PS2EE.
// 2003.02.21 Ver.1.01 Files are deleted only when the internal counter is 0.
//                     File names and line numbers are remembered for memory-related debugging.
//                     Overloaded new/delete.
// 2003.02.20 Ver.1.00 Start creating.
//============================================================================
#ifndef DEBUG_H
#define DEBUG_H

// -------------------- Header file registration
#if defined R5900
#include <stdio.h>
#endif
#include "generic.h"

// Debug (0=no, 1=yes, 2=output only)
#define DEBUG_MODE 2
// Output destination (0=standard output (PS2), debug window (Win32), 1=file)
#define DEBUG_OUTPUT 0
// DirectX version (0=no debugging, only 8 and 9 are available)
#define DEBUG_DIRECTX 0
// Overload New/delete (0=no, 1=yes)
#define DEBUG_NEWDELETE 0

// -------------------- Specifying a link library
#if defined _WIN32
#if DEBUG_DIRECTX == 8
#pragma comment( lib, "dxerr8.lib" )
#elif DEBUG_DIRECTX == 9
#pragma comment( lib, "dxerr9.lib" )
#endif
#endif

// -------------------- Constant definitions
#if defined GBA
static const int DEBUG_MEMORY_MAX = 256;					// Memory allocation monitoring maximum number GBA
#else
static const int DEBUG_MEMORY_MAX = 65536;					// Maximum number of memory allocation monitors Win32/PS2
#endif

#if defined R5900
static const char* const DEBUG_FILE = "host0:report.txt";	// PS2EE debug output file name
#else
static const char* const DEBUG_FILE = "report.txt";			// Debug output file name
#endif
static const U32 DEBUG_10 = 0;
static const U32 DEBUG_16 = 1;
static const U32 DEBUG_DUMP8  = 0x00000001;
static const U32 DEBUG_DUMP16 = 0x00000002;
static const U32 DEBUG_DUMP32 = 0x00000004;

#if DEBUG_MODE == 1
#if DEBUG_NEWDELETE == 1
// -------------------- Override
void* operator new( size_t size, const char* file, int line );
void* operator new[]( size_t size, const char* file, int line );
void operator delete( void* ptr );
void operator delete[]( void* ptr );
#endif
#endif

#if DEBUG_MODE != 0
// -------------------- Class definition
class cDebug {
	public:
		cDebug( void );
		cDebug( int type );
		~cDebug( void );
		// Debug Output
		void DeleteFile( void );
		void SetOutputType( int type );
		void Output( const char* outputstring, ... ) const;
		void OutputDump( const void *address, int width, int height, U32 mode ) const;
		// DirectX
		void OutputDxResult( void *result ) const;
		// Memory Management
		void* Malloc( size_t size, const char *file, int line );
		void Free( void *memblock );
		void MemoryStatus( void );
	private:
		// Number of times staying
		static int StatCount;
		// Output
		int OutputType;
		// Memory Management
		static int MallocHandle;
		static const void *MallocAddress[ DEBUG_MEMORY_MAX ];
		static const char *MallocFile[ DEBUG_MEMORY_MAX ];
		static int MallocLine[ DEBUG_MEMORY_MAX ];
		static size_t MallocSize[ DEBUG_MEMORY_MAX ];
		// Assignment prohibited
		cDebug( cDebug& debug );
		cDebug& operator = ( const cDebug &debug );
};
#endif

// -------------------- Do not debug
#if DEBUG_MODE == 0
#define DEBUGINIT( x )             ;
#define DEBUGFILECLEAR             ;
#define DEBUGOUTMODE( x )          ;
#define DEBUGOUT( x )              ;
#define DEBUGOUTDUMP( a, w, h )    ;
#define DEBUGDX( x )               ;
#define MEMORYSTATE()              ;
// -------------------- All
#elif DEBUG_MODE == 1
#define DEBUGINIT( x )             cDebug DebugMacro( x )
#define DEBUGFILECLEAR             DebugMacro.DeleteFile
#define DEBUGOUTTYPE( x )          DebugMacro.OutputType( x )
#define DEBUGOUT                   DebugMacro.Output
#define DEBUGOUTDUMP( a, w, h )    DebugMacro.OutputDump( a, w, h, DEBUG_DUMP8 )
#define DEBUGDX( x )               DebugMacro.OutputDxResult( (void*)&x )
#define malloc( x )                DebugMacro.Malloc( x, __FILE__, __LINE__ )
#define free( x )                  DebugMacro.Free( x )
#define MEMORYSTATE()              DebugMacro.MemoryStatus()
#if DEBUG_NEWDELETE == 1
#define new                        new( __FILE__, __LINE__ )
#endif

// -------------------- Output only
#elif DEBUG_MODE == 2
#define DEBUGINIT( x )             cDebug DebugMacro( x )
#define DEBUGFILECLEAR             DebugMacro.DeleteFile
#define DEBUGOUTTYPE( x )          DebugMacro.OutputType( x )
#define DEBUGOUT                   DebugMacro.Output
#define DEBUGOUTDUMP( a, w, h )    DebugMacro.OutputDump( a, w, h, DEBUG_DUMP8 )
#define DEBUGDX( x )               DebugMacro.OutputDxResult( (void*)&x )
#define MEMORYSTATE()              ;
#endif

// -------------------- Can be referenced from other files
extern cDebug DebugMacro;

#endif
