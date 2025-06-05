//============================================================================
//  memory.h "Memory Control" Win32 version
//----------------------------------------------------------------------------
//													Programmed by Tago
//----------------------------------------------------------------------------
// 2004.02.23 Ver.1.00 created.
//============================================================================
#ifndef DEFINE_MEMORY
#define DEFINE_MEMORY

// -------------------- Constant definitions
#define MEMORY_MODE 0		// 0=with debug, 1=malloc/free
#define MEMORY_MAX 65536

// Function definition
void InitMemory( void );
void* AllocateMemory( size_t size, const char *file, int line );
void FreeMemory( void *memory, const char *file, int line );
void DisplayMemory( const char *file, int line );

#endif
