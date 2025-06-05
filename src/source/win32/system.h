//============================================================================
//  system.h "Absorbing incompatible parts"
//----------------------------------------------------------------------------
//													Programmed by Keiichi Tago
//============================================================================

// -------------------- Double definition prevention
#ifndef INCLUDE_SYSTEM
#define INCLUDE_SYSTEM

// -------------------- Header file registration
#include <windows.h>
#include "memory.h"
#define PAD_LEFT     0x00000004	// left
#define PAD_DOWN     0x00000002	// under
#define PAD_RIGHT    0x00000008	// right
#define PAD_UP       0x00000001	// above
#define PAD_BUTTON1  0x00000010	// Button 1

// Memory
#define SystemMalloc(x) AllocateMemory(x,__FILE__, __LINE__)
#define SystemFree(x) FreeMemory(x,__FILE__, __LINE__)
#define SystemMemoryStatus() DisplayMemory(__FILE__,__LINE__)
//#define SystemMalloc(x) malloc(x)
//#define SystemFree(x) free(x)
//#define SystemMemoryStatus() ;
// System
bool InitSystem( HWND window_handle );
void ReleaseSystem( void );
// Screen
void Flip( void );
void Clear( void );
void InitRender( void );
void ReleaseRender( void );
void Render( bool fullscreen );
void SetTV( bool tv );
bool GetTV( void );
// CPU
void RunCPU( void );
void ExecRETI( void );
bool ExecInterrupt( void );
void Reset( bool clear );
// I/O Ports
unsigned char ReadIO_CPU( unsigned char laddr, unsigned char haddr );
void WriteIO_CPU( unsigned char laddr, unsigned char haddr, unsigned char val );
// Keyboard
void SetKey( int code );
void ResetKey( int code );
// Virtual Machine Memory
void WriteMemoryByte( unsigned short addr, unsigned char val );
// Printer
void SetPrinter( void );
void WritePhcPrinter( int chr );
// Tape
void SetReadTape( void );
void SetWriteTape( void );
void WriteTape( int data );
int ReadTape( void );
void RewReadTape( void );
// Information column
void SetInformation( bool information );
bool GetInformation( void );
void SetInfomationMode( DWORD mode );
void ResetInfomationMode( DWORD mode );
void SetFps( float fps );
// PSG
void MakeSound( bool fill );
void UpdateSound( void );
void SetVolume( int volume );
int GetVolume( void );
void SetSound( int rate, int sample_count );
int GetSoundBuffer( void );
// Joypad
DWORD GetStick( int player );
// Settings
void LoadSetting( void );
void SaveSetting( void );
void SetScreenScale( int scale );
int GetScreenScale( void );
void SetFullScreen( bool fullscreen );
bool GetFullScreen( void );
void SetMachine( bool type );
bool GetMachine( void );
void SetWarpMode( bool mode );
bool GetWarpMode( void );

#endif
