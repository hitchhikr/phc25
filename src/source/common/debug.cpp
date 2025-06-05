//============================================================================
//  debug.cpp "Debug Control" PS2EE/Win32 version
//----------------------------------------------------------------------------
//													Programmed by Tago
//============================================================================
#include <stdarg.h>
#include <stdlib.h>
#include "debug.h"
#if defined _WIN32
#include <stdio.h>
#include <windows.h>
#if DEBUG_DIRECTX == 8
#include <dxerr8.h>
#elif DEBUG_DIRECTX == 9
#include <dxerr9.h>
#endif
#elif defined R5900
#include <stdio.h>
#include <sifdev.h>
#elif defined GBA
#include <stdio.h>
#include <string.h>
#include "gbaprint.h"
#elif defined SDK_ARM9
#ifndef CODE32
#define CODE32
#endif
#include <nitro.h>
#include <string.h>
#endif
#include "generic.h"

// -------------------- Compile for debugging only
#if DEBUG_MODE != 0

// -------------------- Remove the definition
#if DEBUG_NEWDELETE == 1
#undef new
#endif
#undef malloc
#undef free

// -------------------- Static Region
cDebug DebugMacro( DEBUG_OUTPUT );

#if DEBUG_NEWDELETE == 1
//===========================================================================
//  new
// Note: No exceptions are returned.
//===========================================================================
void* operator new( size_t size, const char* file, int line )
{
	void* ptr = DebugMacro.Malloc( size, file, line );
//	if ( NULL == ptr )
//	{
//		throw 1;
//	}
	return ptr;
}

//===========================================================================
//  new[]
// Note: No exceptions are returned.
//===========================================================================
void* operator new[]( size_t size, const char* file, int line )
{
	void* ptr = DebugMacro.Malloc( size, file, line );
//	if ( NULL == ptr )
//	{
//		throw 1;
//	}
	return ptr;
}

//===========================================================================
//  delete
//===========================================================================
void operator delete( void* ptr )
{
	DebugMacro.Free( ptr );
}

//===========================================================================
//  delete[]
//===========================================================================
void operator delete[]( void* ptr )
{
	DebugMacro.Free( ptr );
}
#endif

//===========================================================================
//  cDebug Class
//===========================================================================

// -------------------- Static Variables
int cDebug::StatCount = 0;
int cDebug::MallocHandle = 0;

#if DEBUG_MODE != 2
const void *cDebug::MallocAddress[ DEBUG_MEMORY_MAX ];
const char *cDebug::MallocFile[ DEBUG_MEMORY_MAX ];
int cDebug::MallocLine[ DEBUG_MEMORY_MAX ];
size_t cDebug::MallocSize[ DEBUG_MEMORY_MAX ];
#endif

//===========================================================================
//  Constructor
//===========================================================================
cDebug::cDebug()
:OutputType( 0 )
{
	StatCount ++;
}

//===========================================================================
//  Constructor
//---------------------------------------------------------------------------
// In  : type
//     :   0 = Standard output (PS2), debug window (Win32)
//     :   1 = File
//===========================================================================
cDebug::cDebug( int type )
:OutputType( type )
{
	if ( 0 == StatCount )
	{
		DeleteFile();
	}
	StatCount ++;
}

//===========================================================================
//  Destructor
//===========================================================================
cDebug::~cDebug()
{
	StatCount --;
}

//===========================================================================
//  Select the output destination
//---------------------------------------------------------------------------
// In  : type
//     :   0 = Standard output (PS2), debug window (Win32)
//     :   1 = File
//===========================================================================
void cDebug::SetOutputType( int type )
{
	OutputType = type;
}

//===========================================================================
//  Delete the debug files
//===========================================================================
void cDebug::DeleteFile( void )
{
	#if defined _WIN32
		#if DEBUG_OUTPUT == 1
			char path[ _MAX_PATH ];
			if( GetModuleFileName( NULL, path, sizeof( path ) ) != 0 )
			{
				char drive[ _MAX_DRIVE ];
				char dir[ _MAX_DIR ];
				_splitpath( path, drive, dir, NULL, NULL );
				strcpy( path, drive );
				strcat( path, dir );
				strcat( path, DEBUG_FILE );
			}
			else
			{
				strcpy( path, DEBUG_FILE );
			}
			unlink( path );
		#endif
	#endif
}

//============================================================================
//	Prints a debug message
//----------------------------------------------------------------------------
//In  : outputstring = Output string
//============================================================================
void cDebug::Output( const char* outputstring, ... ) const
{
	char str[ 1000 ];
	va_list arglist;
	va_start( arglist, outputstring );
	(void)vsprintf( str, outputstring, arglist );
	va_end( arglist );
	if ( 0 == OutputType )
	{
		#if defined _WIN32
			OutputDebugString( (LPCTSTR)str );
		#elif defined GBA
			gbaprint( str );
		#elif defined SDK_ARM9
			OS_Printf( str );
		#else
			printf( str );
		#endif
	}
	else if ( 1 == OutputType )
	{
		#if defined R5900
			int fp;
			fp = sceOpen( DEBUG_FILE, ( SCE_CREAT | SCE_WRONLY ), 0777 );
			if ( fp >= 0 )
			{
				sceLseek( fp, 0, SCE_SEEK_END );
				sceWrite( fp, str, strlen( str ) + 1 );
				sceClose( fp );
			}
		#elif defined _WIN32
			FILE *fp;
			char path[ _MAX_PATH ];
			if( GetModuleFileName( NULL, path, sizeof( path ) ) != 0 )
			{
				char drive[ _MAX_DRIVE ];
				char dir[ _MAX_DIR ];
				_splitpath( path, drive, dir, NULL, NULL );
				strcpy( path, drive );
				strcat( path, dir );
				strcat( path, DEBUG_FILE );
			}
			else
			{
				strcpy( path, DEBUG_FILE );
			}
			// File Open
			fp = fopen( path, "r+" );
			if ( NULL == fp ) {
				// File Creation
				fp = fopen( path, "w+" );
			}
			if ( fp != NULL ) {
				fseek( fp, 0L, SEEK_END );
				fwrite( str, strlen( str ), 1, fp );
				fclose( fp );
			}
		#elif defined GBA
			gbaprint( str );
		#elif defined SDK_ARM9
			OS_Printf( str );
		#else
			FILE *fp;
			// File Open
			fp = fopen( DEBUG_FILE, "r+" );
			if ( NULL == fp )
			{
				// File Creation
				fp = fopen( DEBUG_FILE, "w+" );
			}
			if ( fp != NULL )
			{
				fseek( fp, 0L, SEEK_END );
				fwrite( str, strlen( str ), 1, fp );
				fclose( fp );
			}
		#endif
	}
}

//============================================================================
//	Dumping memory
//----------------------------------------------------------------------------
//In  : mode
//    :   DEBUG_DUMP8 1 byte display
//    :   DEBUG_DUMP16 2 byte display
//    :   DEBUG_DUMP32 4-byte display
//============================================================================
void cDebug::OutputDump( const void *address, int width, int height, U32 mode ) const
{
	int x, y;
	const U8 *dump_address = (U8*)address;
	for ( y = 0; y < height; y ++ )
	{
		Output( "%08X", dump_address );
		Output( " : " );
		for ( x = 0; x < width; x++ )
		{
			if ( mode & DEBUG_DUMP8 )
			{
				Output( "%02X", *(U8*)dump_address );
				dump_address++;
			}
			else if ( mode & DEBUG_DUMP16 )
			{
				Output( "%04X", *(U16*)dump_address );
				dump_address += 2;
			}
			else
			{
				Output( "%08X", *(U32*)dump_address );
				dump_address += 4;
			}
			if ( x != width - 1 )
			{
				Output( " " );
			}
		}
		Output( "\n" );
	}
}

//============================================================================
//	Outputting DirectX errors
//----------------------------------------------------------------------------
//In  : result = A pointer to the DirectX error code.
//============================================================================
void cDebug::OutputDxResult( void *result ) const
{
	#if defined _WIN32
	#if DEBUG_DIRECTX != 0
	char *str;
	char strtemp[ 1000 ];
	#if DEBUG_DIRECTX == 8
	str = (char*)DXGetErrorString8( *(HRESULT*)result );
	#elif DEBUG_DIRECTX == 9
	str = (char*)DXGetErrorString9( *(HRESULT*)result );
	#endif
	sprintf( strtemp, "Return Value = 0x%08X (", (U64)*(HRESULT*)result );
	strcat( strtemp, str );
	strcat( strtemp, ")\n" );
	Output( strtemp );
	#endif
	#else
	void *temp;
	temp = result;
	#endif
}

#if DEBUG_MODE != 2
//============================================================================
//	Memory allocation malloc compatible
//----------------------------------------------------------------------------
//In  : size = number of bytes to allocate
//    : file = the name of the file for which memory is allocated
//    : line = the line where memory is allocated
//============================================================================
void* cDebug::Malloc( size_t size, const char *file, int line )
{
	void *memory;
	memory = malloc( size );
	MallocAddress[ MallocHandle ] = memory;
	MallocSize[ MallocHandle ] = size;
	MallocFile[ MallocHandle ] = file;
	MallocLine[ MallocHandle ] = line;
	MallocHandle ++;
	if ( MallocHandle > DEBUG_MEMORY_MAX )
	{
		MallocHandle = 0;
	}
	return( memory );
}

//============================================================================
//	Memory release free compatible
//============================================================================
void cDebug::Free( void *memblock )
{
	int i;
	for ( i = 0; i < DEBUG_MEMORY_MAX; i ++ )
	{
		if ( MallocAddress[ i ] == memblock )
		{
			MallocAddress[ i ] = NULL;
			break;
		}
	}
	free( memblock );
}

//============================================================================
//	Checking memory management
// *) If the file name or path name contains kanji characters including \, it will not work.
//============================================================================
void cDebug::MemoryStatus( void )
{
	char str[ 1024 ];
	int i;
	size_t sum = 0;
	Output( "*Memory release status\n" );
	for ( i = 0; i < DEBUG_MEMORY_MAX; i ++ )
	{
		if( MallocAddress[ i ] != NULL )
		{
			char file[ 1024 ];
			bool find = false;
			int srcindex = (int)strlen( MallocFile[ i ] ) - 1;
			while( 0 <= srcindex-- )
			{
				if( ( '\\' == MallocFile[ i ][ srcindex ] ) ||
					 ( '/' == MallocFile[ i ][ srcindex ] ) )
				{
					find = true;
					srcindex++;
					break;
				}
			}
			if( find )
			{
				strcpy( file, &MallocFile[ i ][ srcindex ] );
			}
			else
			{
				strcpy( file, MallocFile[ i ] );
			}
            
			sprintf( str, "%d bytes (%p) allocated on line %d of %s have not been released.\n",
					 MallocSize[ i ],
					 MallocAddress[ i ],
					 MallocLine[ i ],
					 file
                    );
			sum += MallocSize[ i ];
			Output( str );
		}
	}
	sprintf( str, "There is %d bytes of unfreed memory.\n", sum );
	Output( str );
}
#endif

// -------------------- Compile for debugging only
#endif
