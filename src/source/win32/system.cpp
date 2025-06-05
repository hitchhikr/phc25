#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "memory.h"
#include "screen.h"
#include "system.h"
#include "sound.h"
#include "monitor.h"
#include "../common/z80.h"
#include "../common/fmgen008/psg.h"
#include "../common/debug.h"

// -------------------- Specifying a link library
#pragma comment( lib, "Winmm.lib" )

#define MODE_KANA    0x00000001
#define MODE_LOADING 0x00000002
#define MODE_SAVING  0x00000004

// System
static HWND WindowHandle;
TIMECAPS TimeCaps;
// Screen
static ScreenClass Screen;
static int DrawPositionX;
static int DrawPositionY;
static bool TV;
// Z80
Z80 z80;
static bool RequestIrq;
static bool RunningIrq;
static int SystemClock;
// PSG
static PSG Psg;
static int SoundCount;		// Number of clocks since last PSG operation
static int SoundRate;		// Sampling rate (44100)
static SoundClass Sound;
static PSG::Sample PsgBuffer[ 44100 * 2 ];	// 1 second
static short PlayBuffer[ 44100 ];			// 1 second
static int BufferSize;
static int MakePointer; // Already created data location
static int Volume;
// Etc
static bool InitFlag = false;
// Japanese 106 Keyboard (Kana=F5、GRPH=F6)
static const int KeyTable[] =
{
	0xE2, 0xBA, VK_DELETE, VK_UP,    'X',        'S',      'W',   '1',
	0xBF, 0xBB, VK_RETURN, VK_DOWN,  'Z',        'A',      'Q',   VK_ESCAPE,
	0,    0xDB, 0xDE,      VK_LEFT,  'V',        'F',      'R',   '3',
	' ',  0xDD, 0xDC,      VK_RIGHT, 'C',        'D',      'E',   '2',
	0,    'P',  '0',       VK_F3,    'N',        'H',      'Y',   '5',
	0,    0xC0, 0xDF,      VK_F4,    'B',        'G',      'T',   '4',
	0,    'O',  '9',       VK_F2,    'M',        'J',      'U',   '6',
	0xBE, 'L',  '8',       VK_F1,    0xBC,       'K',      'I',   '7',
	0,    0,    VK_F5,     0,        VK_CONTROL, VK_SHIFT, VK_F6, 0
};
static int KeyboardMatrix[ 9 ];
// I/O Ports
static unsigned char Port00; // Printer data
static unsigned char Port40; // Graphic & etc
static unsigned char PortC1; // PSG reg
// Memory
static bool EnableExtendMemory = false;
// Tape Control
static bool Remote;
static bool RemoteFirst;
static int RemoteTime;
// Tape Data Control
static int TapeBitCount;
static int TapeDataBit;
static bool StartBit;
static int TapeData;
static int DataCount;
static bool First;
static size_t WriteCount;
static TCHAR ReadTapePath[ MAX_PATH ];
static TCHAR WriteTapePath[ MAX_PATH ];
static int ReadCount;
static int ReadBit;
static int ReadTimingCount;
static unsigned char *ReadData;
static size_t ReadDataSize;
static int ReadDataBit;
static int ReadPhase = 0;
// Printer
static TCHAR PrinterPath[ MAX_PATH ];
// File Selection
static OPENFILENAME OpenFilename;
static TCHAR File[ MAX_PATH ];
static TCHAR Custom[ 256 ];
// Information column
static DWORD Mode;
static float Fps;
static bool Information;
// setting
static int ScreenScale;
static bool FullScreenEnable;
static bool Machine;
static int SoundBuffer;
static bool WarpMode;

// Internal Functions
bool GetLine( FILE *fp, char *key, int *data );

bool InitSystem( HWND window_handle )
{
	int i;
	// System
	WindowHandle = window_handle;
	InitMemory();
	timeGetDevCaps( &TimeCaps, sizeof( TIMECAPS ) );
	timeBeginPeriod( TimeCaps.wPeriodMin );
	// Z80
	z80.Reset();
	RequestIrq = false;
	RunningIrq = false;
	SystemClock = 4 * 1000 * 1000; // 4MHz
	// PSG
	SoundRate = 44100;
	Psg.Reset();
	Psg.SetClock( 1996750, SoundRate ); // 1.99675MHz
	MakePointer = 0;
	SoundBuffer = 10;
	BufferSize = SoundRate / SoundBuffer;
	Volume = 60;
	Sound.Init( window_handle, SoundRate, BufferSize ); // 100ms
	SoundCount = 0;
	Sound.Stop();
	Sound.Play();
	// Virtual Machine Memory
	memset( z80.memory, 0, 65536 );
	memset( &z80.memory[ 0x7800 ], 0xFF, 2048 );
	if( !EnableExtendMemory )
	{
		memset( &z80.memory[ 0x8000 ], 0xFF, 16384 );
	}
	FILE *file;
	file = fopen( "phc25rom.bin", "rb" );
	if( NULL == file )
	{
		DEBUGOUT( "Unable to load phc25rom.bin.\n" );
		return false;
	}
	fread( z80.memory, 24 * 1024, 1, file );
	fclose( file );
	// Checksum
	int j;
	int sum[ 12 ];
	memset( &sum, 0, sizeof( sum ) );
	for( i = 0; i < 12; i ++ )
	{
		for( j = 0; j < 2048; j ++ )
		{
			sum[ i ] += z80.memory[ i * 2048 + j ];
			if( sum[ i ] > 65535 )
			{
				sum[ i ] -= 65536;
			}
		}
		DEBUGOUT( "block = %d, sum = %d\n", i, sum[ i ] ); 
	}
	// screen
	Screen.Create( WindowHandle, &z80.memory[ 0x6000 ], &z80.memory[ 0x5000 ] );
	Machine = true;
	TV = false;
	Screen.SetBlot( TV );
	// keyboard
	for( i = 0; i < 9; i ++ )
	{
		KeyboardMatrix[ i ] = 0xFF;
	}
	// Tape Control
	Remote = false;
	RemoteFirst = false;
	RemoteTime = 0;
	InitFlag = true;
	ReadData = 0;
	ReadCount = -1;
	ReadBit = 0;
	ReadTimingCount = 0;
	ReadPhase = 0;
	// Printer
	strcpy( PrinterPath, "phc25_printer.txt" );
	// Information column
	Mode = 0;
	WriteCount = 0;
	Information = true;
	Screen.SetInfomation( Information );
	// default settings
	ScreenScale = 1;
	FullScreenEnable = false;
    WarpMode = false;
	LoadSetting();
	return true;
}

void ReleaseSystem( void )
{
	SaveSetting();
	if( ReadData )
	{
		SystemFree( ReadData );
		ReadData = 0;
	}
	Sound.Release();
	timeEndPeriod( TimeCaps.wPeriodMin );
	SystemMemoryStatus();
}

//-------------------- screen
void Flip( void )
{
}

void Clear( void )
{
}

void InitRender( void )
{
}

void ReleaseRender( void )
{
}

void Render( bool fullscreen )
{
	if( InitFlag )
	{
		unsigned char *vram;
		HDC hdc;
		PAINTSTRUCT ps;
		hdc = BeginPaint( WindowHandle, &ps );
		vram = z80.memory + 0x6000;
		if( Port40 & 0x80 )
		{
			int select_color;
			select_color = ( Port40 & 0x40 ) >> 6;
			// Graphics Mode
			if( Port40 & 0x20 )
			{
				// SCREEN 4
				Screen.Render( MODE_GRAPH4, vram, select_color );
			}
			else
			{
				// SCREEN 3
				Screen.Render( MODE_GRAPH3, vram, select_color );
			}
		}
		else
		{
			// Text mode
			Screen.Render( MODE_TEXT, vram );
		}
		// Information column
		if( Mode & MODE_KANA )
		{
			Screen.DrawFont( 28, 16, 0x96, -1 );
			Screen.DrawFont( 29, 16, 0xE5, -1 );
			Screen.DrawFont( 30, 16, ' ', -1 );
			Screen.DrawFont( 31, 16, ' ', -1 );
		}
		else
		{
			Screen.DrawFont( 28, 16, 'A', -1 );
			Screen.DrawFont( 29, 16, 'B', -1 );
			Screen.DrawFont( 30, 16, 'C', -1 );
			Screen.DrawFont( 31, 16, ' ', -1 );
		}
		if( Mode & MODE_LOADING )
		{
			Screen.DrawFont( 0, 16, 'L', -1 );
			Screen.DrawFont( 1, 16, 'o', -1 );
			Screen.DrawFont( 2, 16, 'a', -1 );
			Screen.DrawFont( 3, 16, 'd', -1 );
			Screen.DrawFont( 4, 16, 'i', -1 );
			Screen.DrawFont( 5, 16, 'n', -1 );
			Screen.DrawFont( 6, 16, 'g', -1 );
            if(WarpMode)
            {
                SystemClock = 400 * 1000 * 1000; // 400MHz
            }
		}
		else if( Mode & MODE_SAVING )
		{
			Screen.DrawFont( 0, 16, 'S', -1 );
			Screen.DrawFont( 1, 16, 'a', -1 );
			Screen.DrawFont( 2, 16, 'v', -1 );
			Screen.DrawFont( 3, 16, 'i', -1 );
			Screen.DrawFont( 4, 16, 'n', -1 );
			Screen.DrawFont( 5, 16, 'g', -1 );
            if(WarpMode)
            {
                SystemClock = 400 * 1000 * 1000; // 400MHz
            }
		}
		else
		{
			Screen.DrawFont( 0, 16, 'R', -1 );
			Screen.DrawFont( 1, 16, 'e', -1 );
			Screen.DrawFont( 2, 16, 'a', -1 );
			Screen.DrawFont( 3, 16, 'd', -1 );
			Screen.DrawFont( 4, 16, 'y', -1 );
			Screen.DrawFont( 5, 16, ' ', -1 );
			Screen.DrawFont( 6, 16, ' ', -1 );
            SystemClock = 4 * 1000 * 1000; // 4MHz
		}
		Screen.DrawFont( 7, 16, ' ', -1 );
		char counter[ 6 ] = "00000";
		if( -1 == ReadCount )
		{
			sprintf( counter, "%05d", 0 );
		}
		else if( ReadCount > 99999 )
		{
			sprintf( counter, "%05d", 99999 );
		}
		else
		{
			sprintf( counter, "%05d", ReadCount );
		}
		Screen.DrawFont( 7, 16, ' ', -1 );
		Screen.DrawFont( 8, 16, 256, -1 );
		Screen.DrawFont( 9, 16, counter[ 0 ], -1 );
		Screen.DrawFont( 10, 16, counter[ 1 ], -1 );
		Screen.DrawFont( 11, 16, counter[ 2 ], -1 );
		Screen.DrawFont( 12, 16, counter[ 3 ], -1 );
		Screen.DrawFont( 13, 16, counter[ 4 ], -1 );
		if( WriteCount < 0 )
		{
			sprintf( counter, "%05d", 0 );
		}
		else if( ReadCount > 99999 )
		{
			sprintf( counter, "%05d", 99999 );
		}
		else
		{
			sprintf( counter, "%05d", WriteCount );
		}
		Screen.DrawFont( 14, 16, ' ', -1 );
		Screen.DrawFont( 15, 16, 257, -1 );
		Screen.DrawFont( 16, 16, counter[ 0 ], -1 );
		Screen.DrawFont( 17, 16, counter[ 1 ], -1 );
		Screen.DrawFont( 18, 16, counter[ 2 ], -1 );
		Screen.DrawFont( 19, 16, counter[ 3 ], -1 );
		Screen.DrawFont( 20, 16, counter[ 4 ], -1 );
		sprintf( counter, "%02.01f", Fps );
		Screen.DrawFont( 21, 16, ' ', -1 );
		Screen.DrawFont( 22, 16, 258, -1 );
		Screen.DrawFont( 23, 16, counter[ 0 ], -1 );
		Screen.DrawFont( 24, 16, counter[ 1 ], -1 );
		Screen.DrawFont( 25, 16, counter[ 2 ], -1 );
		Screen.DrawFont( 26, 16, counter[ 3 ], -1 );
		Screen.DrawFont( 27, 16, ' ', -1 );
		Screen.Draw( hdc, fullscreen );
		EndPaint( WindowHandle, &ps );
	}
}

void SetTV( bool tv )
{
	TV = tv;
	Screen.SetBlot( TV );
}

bool GetTV( void )
{
	return TV;
}

//-------------------- Run the CPU for one frame
void RunCPU( void )
{
	int i;
	int clock_sum = 0;
    int divider;
    int lines;

    // PAL
    lines = 312;
    divider = 50;
    if(GetMachine() == false)
    {
        // NTSC
        lines = 262;
        divider = 60;
    }
	for( i = 0; i < lines; i ++ )
	{
		DrawPositionY = i;
		int clock_count = 0;
		if( i == 192 )
		{
			RequestIrq = true;
		}

		while( clock_count < ( SystemClock / divider / lines ) )
		{
			int result_clock;
			if( RequestIrq )
			{
				if( ExecInterrupt() )
				{
					RequestIrq = false;
				}
			}
            if(Z80_Pause)
            {
                goto No_Op;
            }
			result_clock = z80.Run();
			if( 0 == result_clock )
			{
				break;
			}
			DrawPositionX = clock_count;
			clock_count += result_clock;
			clock_sum += result_clock;
			SoundCount += result_clock;
			if( Remote )
			{
				if( RemoteFirst )
				{
					RemoteFirst = false;
					RemoteTime = 0;
				}
				else
				{
					RemoteTime += result_clock;
				}
			}
		}
		UpdateSound();
	}
No_Op:;
}

void ExecRETI( void )
{
	RunningIrq = false;
	if( RequestIrq )
	{
		ExecInterrupt();
	}
}

bool ExecInterrupt( void )
{
	if( false == z80.EnableIRQ() )
	{
		return false;
	}
	if( RunningIrq )
	{
		return false;
	}
	z80.DoIRQ( 0 );
	RunningIrq = true;
	return true;
}

void Reset( bool clear )
{
	int i;
	z80.Reset();
	RequestIrq = false;
	RunningIrq = false;
	// Virtual Machine Memory
	memset( &z80.memory[ 0x7800 ], 0xFF, 2048 );
	if( !EnableExtendMemory )
	{
        if( clear )
        {
		    memset( &z80.memory[ 0x8000 ], 0xFF, 16384 );
        }
	}
	// keyboard
	for( i = 0; i < 9; i ++ )
	{
		KeyboardMatrix[ i ] = 0xFF;
	}
	// Tape Control
    if( clear )
    {
	    Remote = false;
	    RemoteFirst = false;
	    RemoteTime = 0;
	    InitFlag = true;
	    ReadData = 0;
        ReadCount = -1;
	    ReadBit = 0;
	    ReadTimingCount = 0;
	    ReadPhase = 0;
    }
}

//-------------------- I/O Ports
unsigned char ReadIO_CPU( unsigned char laddr, unsigned char haddr )
{
	unsigned char return_code = 0xFF;
	if( 0x40 == laddr )
	{
		return_code = 0;
		// bit4 Vertical return line
		if( DrawPositionY < 192 )
		{
			return_code |= 0x10;
		}
		// bit5 Cassette Read signal
		if( Remote )
		{
			SetInfomationMode( MODE_LOADING );
			if( !First )
			{
				ReadDataBit = ReadTape();
				First = true;
			}
			if( RemoteTime > 790 )
			{
				ReadDataBit = ReadTape();
//				RemoteTime -= 790; GFE
				RemoteTime=0;
			}
			if( ReadDataBit )
			{
				return_code |= 0x20;
			}
		}
		// bit6 Printer Busy signal
		// bit7 horizontal retrace (tentative)
		if( DrawPositionX < 224 )
		{
			return_code |= 0x80;
		}
	}
	else if( ( 0x80 <= laddr ) && ( 0x88 >= laddr ) )
	{
		// keyboard
		return_code = KeyboardMatrix[ laddr - 0x80 ];
	}
	else if( 0xC1 == laddr )
	{
		// PSG Data
		if( PortC1 < 14 )
		{
			return_code = Psg.GetReg( PortC1 );
		}
		else if( PortC1 < 16 )
		{
			return_code = ~( unsigned char )GetStick( PortC1 - 14 );
		}
	}
	return return_code;
}

void WriteIO_CPU( unsigned char laddr, unsigned char haddr, unsigned char val )
{
	if( 0x00 == laddr )
	{
		// Printer Output
		Port00 = val;
	}
	else if( 0x40 == laddr )
	{
		// Bit 0 Cassette Write Control
		if( ( Remote ) && ( !First ) && ( val & 1 ) )
		{
			First = true;
		}
		if( ( Remote ) && ( First ) )
		{
			SetInfomationMode( MODE_SAVING );
//			DEBUGOUT( "write time = %d, data = %d\n", RemoteTime, val & 1 );
			TapeDataBit <<= 1;
			TapeDataBit |= ( val & 1 );
			TapeBitCount ++;
			if( TapeBitCount > 3 )
			{
				// Bit detection
				if( 0xC == TapeDataBit )		// 1100
				{
					if( StartBit )
					{
						// 0 Detected
						TapeData >>= 1;
						DataCount ++;
					}
					else
					{
						// StartBit detection
						StartBit = true;
						TapeData = 0;
						DataCount = 0;
					}
				}
				else if( 0xA == TapeDataBit )	// 1010
				{
					if( StartBit )
					{
						// 1 Detection
						TapeData >>= 1;
						TapeData |= 0x80;
						DataCount ++;
					}
				}
				if( DataCount > 7 )
				{
					WriteTape( TapeData );
					StartBit = false;
					DataCount = 0;
				}
				TapeBitCount = 0;
				TapeDataBit = 0;
			}
			RemoteTime = 0;
		}
		// Bit 1 Cassette Remote Control
		if( Port40 & 0x02 )
		{
			if( !( val & 0x02 ) )
			{
				// Hi → Lo = ON
				Remote = true;
				RemoteFirst = true;
				RemoteTime = 0;
				StartBit = false;
				TapeBitCount = 0;
				TapeDataBit = 0;
				First = false;
				ReadBit = 0;
				ReadTimingCount = 0;
				if( 2 == ReadPhase )
				{
					ReadCount = 16;
					ReadPhase = 1;
				}
			}
		}
		if( !( Port40 & 0x02 ) )
		{
			if( val & 0x02 )
			{
				// Lo / Hi = OFF
				Remote = false;
				RemoteTime = 0;
				ResetInfomationMode( MODE_LOADING );
				ResetInfomationMode( MODE_SAVING );
			}
		}
		// bit2 Kana key (LOCK key in overseas version)
		if( val & 0x04 )
		{
			ResetInfomationMode( MODE_KANA );
		}
		else
		{
			SetInfomationMode( MODE_KANA );
		}
		// bit3 Printer output (Hi / Lo = output)
		if( Port40 & 0x08 )
		{
			if( !( val & 0x08 ) )
			{
				WritePhcPrinter( Port00 );
			}
		}
		Port40 = val;
	}
	else if( 0xC0 == laddr )
	{
		// PSG Data
		Psg.SetReg( PortC1, val );
		UpdateSound();
	}
	else if( 0xC1 == laddr )
	{
		// PSG Registers
		PortC1 = val;
	}
}

//-------------------- keyboard
void SetKey( int code )
{
	int x;
	int y;
	for( y = 0; y < 9; y ++ )
	{
		for( x = 0; x < 8; x ++ )
		{
			if( KeyTable[ y * 8 + x ] == code )
			{
				KeyboardMatrix[ y ] &= ( ~( 1 << ( 7 - x ) ) );
			}
		}
	}
}

void ResetKey( int code )
{
	int x;
	int y;
	for( y = 0; y < 9; y ++ )
	{
		for( x = 0; x < 8; x ++ )
		{
			if( KeyTable[ y * 8 + x ] == code )
			{
				KeyboardMatrix[ y ] |= ( 1 << ( 7 - x ) );
			}
		}
	}
}

//-------------------- Virtual Machine Memory
void WriteMemoryByte( unsigned short addr, unsigned char val )
{
	if( ( addr >= 0x6000 ) && ( addr <= 0x77FF ) )
	{
		// VRAM
		z80.memory[ addr ] = val;
	}
	else if( ( addr >= 0x8000 ) && ( addr <= 0xBFFF ) )
	{
		// Extend RAM
		if( EnableExtendMemory )
		{
			z80.memory[ addr ] = val;
		}
	}
	else if( ( addr >= 0xC000 ) && ( addr <= 0xFFFF ) )
	{
		// RAM
		z80.memory[ addr ] = val;
	}
}

void SetExtendMemory( bool enable )
{
	EnableExtendMemory = enable;
}

//-------------------- PHC-25 printer
void SetPrinter( void )
{
	BOOL result;
	memset( &OpenFilename, 0, sizeof( OPENFILENAME ) );
	OpenFilename.lStructSize = sizeof( OPENFILENAME );
	OpenFilename.hwndOwner = WindowHandle;
	OpenFilename.lpstrFilter = TEXT("PHC-25 printer data files {*.txt}\0*.txt\0");
	OpenFilename.lpstrCustomFilter = Custom;
	OpenFilename.nMaxCustFilter = 256;
	OpenFilename.nFilterIndex = 0;
	OpenFilename.lpstrFile = File;
	OpenFilename.nMaxFile = MAX_PATH;
	OpenFilename.Flags = 0;
	OpenFilename.lpstrFileTitle = "";
	OpenFilename.nMaxFileTitle = 0;
	result = GetOpenFileName( &OpenFilename );
	if ( result )
	{
		strcpy( PrinterPath, OpenFilename.lpstrFile );
		if( stricmp( &PrinterPath[ strlen( WriteTapePath ) - 4 ], ".txt" ) )
		{
			strcat( PrinterPath, ".txt" );
		}
		WriteCount = 0;
	}
}

void WritePhcPrinter( int chr )
{
	if( 0x0D == chr )
	{
		return;
	}
	FILE *fp;
	// File Open
	fp = fopen( PrinterPath, "r+" );
	if ( NULL == fp )
	{
		// File Creation
		fp = fopen( PrinterPath, "w+" );
	}
	if ( fp != NULL )
	{
		fseek( fp, 0L, SEEK_END );
		fwrite( &chr, 1, 1, fp );
		fclose( fp );
	}
}

//-------------------- tape
void SetReadTape( void )
{
	BOOL result;
	memset( &OpenFilename, 0, sizeof( OPENFILENAME ) );
	OpenFilename.lStructSize = sizeof( OPENFILENAME );
	OpenFilename.hwndOwner = WindowHandle;
	OpenFilename.lpstrFilter = TEXT("PHC-25 tape image files {*.phc}\0*.phc\0");
	OpenFilename.lpstrCustomFilter = Custom;
	OpenFilename.nMaxCustFilter = 256;
	OpenFilename.nFilterIndex = 0;
	OpenFilename.lpstrFile = File;
	OpenFilename.nMaxFile = MAX_PATH;
	OpenFilename.Flags = OFN_FILEMUSTEXIST;
	OpenFilename.lpstrFileTitle = "";
	OpenFilename.nMaxFileTitle = 0;
	result = GetOpenFileName( &OpenFilename );
	if ( result )
	{
		strcpy( ReadTapePath, OpenFilename.lpstrFile );
		if( ReadData )
		{
			SystemFree( ReadData );
			ReadData = 0;
		}
		// File Open
		FILE *fp;
		fp = fopen( ReadTapePath, "rb" );
		if ( fp != NULL )
		{
			fseek( fp, 0L, SEEK_END );
			ReadDataSize = ftell( fp );
			ReadData = ( unsigned char * )SystemMalloc( ReadDataSize );
			fseek( fp, 0L, SEEK_SET );
			fread( ReadData, ReadDataSize, 1, fp );
			fclose( fp );
		}
		RewReadTape();
	}
}

void SetWriteTape( void )
{
	BOOL result;
	memset( &OpenFilename, 0, sizeof( OPENFILENAME ) );
	OpenFilename.lStructSize = sizeof( OPENFILENAME );
	OpenFilename.hwndOwner = WindowHandle;
	OpenFilename.lpstrFilter = TEXT("PHC-25 tape image files {*.phc}\0*.phc\0");
	OpenFilename.lpstrCustomFilter = Custom;
	OpenFilename.nMaxCustFilter = 256;
	OpenFilename.nFilterIndex = 0;
	OpenFilename.lpstrFile = File;
	OpenFilename.nMaxFile = MAX_PATH;
	OpenFilename.Flags = 0;
	OpenFilename.lpstrFileTitle = "";
	OpenFilename.nMaxFileTitle = 0;
	result = GetOpenFileName( &OpenFilename );
	if ( result )
	{
		strcpy( WriteTapePath, OpenFilename.lpstrFile );
		if( stricmp( &WriteTapePath[ strlen( WriteTapePath ) - 4 ], ".phc" ) )
		{
			strcat( WriteTapePath, ".phc" );
		}
		FILE *fp;
		// File Open
		fp = fopen( WriteTapePath, "r+" );
		if ( NULL != fp )
		{
			fclose( fp );
			if( IDYES == MessageBox( WindowHandle, "do you want to replace file?", "OK", MB_YESNO ) )
			{
				// Delete a file
				unlink( WriteTapePath );
			}
			else
			{
				WriteTapePath[ 0 ] = '\0';
			}
		}
		WriteCount = 0;
	}
}

void WriteTape( int data )
{
	FILE *fp;
	unsigned char write_data;
	write_data = ( unsigned char )data;
	// File Open
	fp = fopen( WriteTapePath, "rb+" );
	if ( NULL == fp )
	{
		// File Creation
		fp = fopen( WriteTapePath, "wb+" );
	}
	if ( fp != NULL )
	{
		fseek( fp, 0L, SEEK_END );
		WriteCount = ftell( fp );
		fwrite( &write_data, 1, 1, fp );
		fclose( fp );
	}
}

void RewReadTape( void )
{
	ReadCount = 0;
	ReadBit = 0;
	ReadTimingCount = 0;
	ReadPhase = 0;
}

int ReadTape( void )
{
	int ret_code = 0;
	if( 0 == ReadData )
	{
		return 0;
	}
	if( -1 == ReadCount )
	{
		return 0;
	}
	if( 0 == ReadPhase )
	{
		if( ReadTimingCount < 1000 ) // 0…
		{
			// Silence
			ret_code = 0;
		}
		else if( ReadTimingCount < 25000 ) // 10…
		{
			// Header approx. 4.8 seconds
			ret_code = 1 - ( ReadTimingCount & 1 );
		}
		else
		{
			ReadPhase = 2;
		}
	}
	if( 1 == ReadPhase )
	{
		if( ReadTimingCount < 3200 ) // 10…
		{
			// Subheader approx. 0.64 seconds
			ret_code = 1 - ( ReadTimingCount & 1 );
		}
		else
		{
			ReadPhase = 3;
		}
	}
	if( ReadPhase > 1 )
	{
		// Output bit data determination
		int data;
		if( 0 == ReadBit )
		{
			// Start bit
			data = 0;
		}
		else if ( ReadBit > 8 )
		{
			// Stop bit
			data = 1;
		}
		else
		{
			// Data bit
			data = ( ReadData[ ReadCount ] >> ( ReadBit - 1 ) ) & 1;
		}
		// Converts bit data into a recording signal
		if( 0 == ( ReadTimingCount % 4 ) )
		{
			ret_code = 1;
		}
		else if( 1 == ( ReadTimingCount % 4 ) )
		{
			ret_code = 1 - data;
		}
		else if( 2 == ( ReadTimingCount % 4 ) )
		{
			ret_code = data;
		}
		else
		{
			ret_code = 0;
			ReadBit ++;
			if( ReadBit > 12 )
			{
				ReadBit = 0;
				ReadCount ++;
				if( ( 2 == ReadPhase ) && ( ReadCount > 16 ) )
				{
					ReadPhase = 1;
					ReadTimingCount = -1;
				}
				if( ReadCount >= (int)ReadDataSize )
				{
					ReadCount = -1;
				}
			}
		}
	}
	ReadTimingCount ++;
	return ret_code;
}

void MakeSound( bool fill )
{
	int sample_count;
	int i;
	if( fill )
	{
		sample_count = BufferSize - MakePointer;
	}
	else
	{
		int clock_count;
		clock_count = SoundCount;
		if( clock_count <= 0 )
		{
			return;
		}
		sample_count = clock_count / 91; // 1 sample is about 91 clocks (at 4MHz)
		if( sample_count < 1 )
		{
			return;
		}
		if( ( MakePointer + sample_count ) >= BufferSize )
		{
			sample_count = BufferSize - MakePointer;
		}
	}
	if( sample_count < 1 )
	{
		return;
	}
	memset( PsgBuffer, 0, sizeof( PSG::Sample ) * sample_count * 2 );
	Psg.Mix( PsgBuffer, sample_count );
	for( i = 0; i < sample_count; i ++ )
	{
		int set_buffer;
		set_buffer = PsgBuffer[ i * 2 ] * ( Volume * 1000 / 16666 );
		if( set_buffer > 32767 )
		{
			set_buffer = 32767;
		}
		else if( set_buffer < -32767 )
		{
			set_buffer = -32767;
		}
		PlayBuffer[ MakePointer + i ] = (short)set_buffer;
	}
	MakePointer += sample_count;
	SoundCount = 0;
}

//-------------------- PSG
void UpdateSound( void )
{
	// Requesting sound data?
	if( Sound.GetStatus() )
	{
		if( BufferSize > MakePointer )
		{
			MakeSound( true );
		}
		Sound.AddData( PlayBuffer );
		MakePointer = 0;
	}
	else
	{
		MakeSound( false );
	}
}

void SetVolume( int volume )
{
	Volume = volume;
}

int GetVolume( void )
{
	return Volume;
}

void SetSound( int rate, int buffer_size )
{
	const int buffer_size_table[] =
	{
		100, 67, 50, 40, 33, 29, 25, 22, 20, 13, 10, 8, 7, 5, 2
	};
	SoundBuffer = buffer_size;
	Sound.Release();
	BufferSize = SoundRate / buffer_size_table[ SoundBuffer ];
	Sound.Init( WindowHandle, rate, BufferSize );
	SoundCount = 0;
	Sound.Stop();
	Sound.Play();
}

int GetSoundBuffer( void )
{
	return SoundBuffer;
}

//-------------------- Information column
void SetInformation( bool information )
{
	Information = information;
	Screen.SetInfomation( Information );
}

bool GetInformation( void )
{
	return Information;
}

void SetInfomationMode( DWORD mode )
{
	Mode |= mode;
}

void ResetInfomationMode( DWORD mode )
{
	Mode &= ~mode;
}

void SetFps( float fps )
{
	Fps = fps;
}

//-------------------- Joypad
DWORD GetStick( int player )
{
	DWORD RetCode = 0;
	JOYINFOEX Joy;
	Joy.dwSize = sizeof( JOYINFOEX );
	Joy.dwFlags = JOY_RETURNALL;
	if( 0 == player )
	{
		if ( joyGetPosEx( JOYSTICKID1, &Joy ) != JOYERR_NOERROR )
		{
			return 0;
		}
	}
	else
	{
		if ( joyGetPosEx( JOYSTICKID1, &Joy ) != JOYERR_NOERROR )
		{
			return 0;
		}
	}
	if ( Joy.dwXpos > 0x8000 + 16384 )
	{
		RetCode |= PAD_RIGHT;
	}
	if ( Joy.dwXpos < 0x8000 - 16384 )
	{
		RetCode |= PAD_LEFT;
	}
	if ( Joy.dwYpos > 0x8000 + 16384 )
	{
		RetCode |= PAD_DOWN;
	}
	if ( Joy.dwYpos < 0x8000 - 16384 )
	{
		RetCode |= PAD_UP;
	}
	if( Joy.dwButtons )
	{
		RetCode |= PAD_BUTTON1;
	}
	return RetCode;
}

//-------------------- Settings
void LoadSetting( void )
{
	char path[ _MAX_PATH ];
	if( GetModuleFileName( NULL, path, sizeof( path ) ) != 0 )
	{
		char drive[ _MAX_DRIVE ];
		char dir[ _MAX_DIR ];
		_splitpath( path, drive, dir, NULL, NULL );
		strcpy( path, drive );
		strcat( path, dir );
		strcat( path, "phc25.ini" );
	}
	else
	{
		strcpy( path, "phc25.ini" );
	}
	FILE *fp;
	fp = fopen( path, "r" );
	if( NULL == fp )
	{
		return;
	}
	char key[ 100 ];
	int data;
	memset( key, 0, sizeof( key ) );
	data = 0;
	while( GetLine( fp, key, &data ) )
	{
		if( 0 == strcmp( "ScreenScale", key ) )
		{
			if( ( data > 0 ) && ( data < 5 ) )
			{
				ScreenScale = data;
			}
		}
		else if( 0 == strcmp( "FullScreenEnable", key ) )
		{
			FullScreenEnable = false;
			if( 1 == data )
			{
				FullScreenEnable = true;
			}
		}
		else if( 0 == strcmp( "MachineType", key ) )
		{
			Machine = false;
			if( 1 == data )
			{
				Machine = true;
			}
		}
		else if( 0 == strcmp( "StatusEnable", key ) )
		{
			Information = false;
			if( 1 == data )
			{
				Information = true;
			}
		}
		else if( 0 == strcmp( "DisplayMode", key ) )
		{
			TV = false;
			if( 1 == data )
			{
				TV = true;
			}
		}
		else if( 0 == strcmp( "SoundVolume", key ) )
		{
			if( ( data >= 0 ) && ( data <= 100 ) )
			{
				Volume = data;
			}
		}
		else if( 0 == strcmp( "SoundBuffer", key ) )
		{
			if( ( data > 0 ) && ( data <= 100 ) )
			{
				SoundBuffer = data;
			}
		}
		else if( 0 == strcmp( "WarpMode", key ) )
		{
			WarpMode = false;
			if( 1 == data )
			{
				WarpMode = true;
			}
		}
		memset( key, 0, sizeof( key ) );
		data = 0;
	}
	fclose( fp );
}

bool GetLine( FILE *fp, char *key, int *data )
{
	int phase = 0;
	int count = 0;
	*data = 0;
	while( 1 )
	{
		int chr;
		if( feof( fp ) )
		{
			return false;
		}
		chr = getc( fp );
		if( '\x0A' == chr )
		{
			break;
		}
		if( '\x0D' == chr )
		{
			continue;
		}
		if( ' ' == chr )
		{
			continue;
		}
		if( '\t' == chr )
		{
			continue;
		}
		if( '=' == chr )
		{
			phase = 1;
			continue;
		}
		if( 0 == phase )
		{
			// Find the key
			*key = chr;
			key ++;
		}
		else
		{
			// Finding Data
			if( ( chr >= '0' ) && ( chr <= '9' ) )
			{
				*data *= 10;
				*data += ( chr - '0' );
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void SaveSetting( void )
{
	char path[ _MAX_PATH ];
	if( GetModuleFileName( NULL, path, sizeof( path ) ) != 0 )
	{
		char drive[ _MAX_DRIVE ];
		char dir[ _MAX_DIR ];
		_splitpath( path, drive, dir, NULL, NULL );
		strcpy( path, drive );
		strcat( path, dir );
		strcat( path, "phc25.ini" );
	}
	else
	{
		strcpy( path, "phc25.ini" );
	}
	FILE *fp;
	char str[ 100 ];
	fp = fopen( path, "w" );
	if( NULL == fp )
	{
		return;
	}
	sprintf( str, "ScreenScale = %d\n", ScreenScale );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "FullScreenEnable = %d\n", FullScreenEnable );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "MachineType = %d\n", Machine );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "StatusEnable = %d\n", Information );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "DisplayMode = %d\n", TV );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "SoundVolume = %d\n", Volume );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "SoundBuffer = %d\n", SoundBuffer );
	fwrite( str, strlen( str ), 1, fp );
	sprintf( str, "WarpMode = %d\n", WarpMode );
	fwrite( str, strlen( str ), 1, fp );
	fclose( fp );
}

void SetScreenScale( int scale )
{
	ScreenScale = scale;
}

int GetScreenScale( void )
{
	return ScreenScale;
}

void SetFullScreen( bool fullscreen )
{
	FullScreenEnable = fullscreen;
}

bool GetFullScreen( void )
{
	return FullScreenEnable;
}

void SetMachine( bool type )
{
	Machine = type;
}

bool GetMachine( void )
{
	return Machine;
}

void SetWarpMode( bool mode )
{
	WarpMode = mode;
}

bool GetWarpMode( void )
{
	return WarpMode;
}

