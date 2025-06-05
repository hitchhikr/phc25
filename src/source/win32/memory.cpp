//============================================================================
//  memory.cpp "Memory Control" Win32 version
//----------------------------------------------------------------------------
//													Programmed by Tago
//============================================================================
#include <stdio.h>
#include <stdlib.h>
#include "../common/debug.h"
#include "memory.h"

#if 0 == MEMORY_MODE

// Variables definition
static int MallocHandle;
static const void *MallocAddress[ MEMORY_MAX ];
static const char *MallocFile[ MEMORY_MAX ];
static int MallocLine[ MEMORY_MAX ];
static size_t MallocSize[ MEMORY_MAX ];

//============================================================================
//	Memory management initialization
//============================================================================
void InitMemory( void )
{
}

//============================================================================
//	Memory Allocation
//----------------------------------------------------------------------------
//In  : size = number of bytes to allocate
//    : file = the name of the file for which memory is allocated
//    : line = the line where memory is allocated
//============================================================================
void* AllocateMemory( size_t size, const char *file, int line )
{
	void *memory;
	memory = malloc( size );
	MallocAddress[ MallocHandle ] = memory;
	MallocSize[ MallocHandle ] = size;
	MallocFile[ MallocHandle ] = file;
	MallocLine[ MallocHandle ] = line;
	MallocHandle++;
	if ( MallocHandle > MEMORY_MAX )
	{
		MallocHandle = 0;
	}
	return( memory );
}

//============================================================================
//	Memory release
//============================================================================
void FreeMemory( void *memory, const char *file, int line )
{
	int i;
	for ( i = 0; i < MEMORY_MAX; i ++ )
	{
		if ( MallocAddress[ i ] == memory )
		{
			MallocAddress[ i ] = NULL;
			break;
		}
	}
	free( memory );
}

//============================================================================
//	Checking memory management
// *) If the file name or path name contains kanji characters including \, it will not work.
//============================================================================
void DisplayMemory( const char *file, int line )
{
	int i;
	size_t sum = 0;
	DEBUGOUT( "*Memory release status\n" );
	DEBUGOUT( "file=%s\n", file );
	DEBUGOUT( "line=%d\n", line );
	for ( i = 0; i < MEMORY_MAX; i ++ )
	{
		if ( NULL == MallocAddress[ i ] )
		{
			continue;
		}
		char file[ 1024 ];
		bool find = false;
		int srcindex = (int)strlen( MallocFile[ i ] ) - 1;
		while( 0 <= srcindex -- )
		{
			if ( ( '\\' == MallocFile[ i ][ srcindex ] ) ||
				 ( '/' == MallocFile[ i ][ srcindex ] ) )
			{
				find = true;
				srcindex ++;
				break;
			}
		}
		if ( find )
		{
			strcpy( file, &MallocFile[ i ][ srcindex ] );
		}
		else
		{
			strcpy( file, MallocFile[ i ] );
		}
		DEBUGOUT( "%d bytes (%p) allocated on line %d of %s have not been released.\n",
				 MallocSize[ i ],
				 MallocAddress[ i ],
				 MallocLine[ i ],
				 file
                );
		sum += MallocSize[ i ];
	}
	DEBUGOUT( "There is %d bytes of unfreed memory.\n", sum );
}
#endif
