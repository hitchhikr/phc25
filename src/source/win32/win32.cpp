//============================================================================
//  win32.cpp "Window control program"
//----------------------------------------------------------------------------
//													Programmed by Keiichi Tago
//----------------------------------------------------------------------------
// *win32.h, win32.rc, and resource.h are required.
//----------------------------------------------------------------------------
// 2004.02.13 Modified R.P.G's Win32.cpp to make it general-purpose.
//============================================================================
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/debug.h"
#include "../win32/main.h"
#include "system.h"
#include "win32.h"
#include "resource.h"
#include "monitor.h"

// -------------------- Specifying a link library
#pragma comment( lib, "Winmm.lib" )

#define MAX_LOADSTRING 100

#define MENU_SCREENSIZE  0
#define MENU_MACHINE     1
#define MENU_VOLUME      2
#define MENU_SOUNDBUFFER 3

//-------------------- Global Variables
HINSTANCE Instance;						// Current Instance
HWND Wnd;								// Window handle
TCHAR Title[ MAX_LOADSTRING ];			// Title Bar Text
TCHAR WindowClass[ MAX_LOADSTRING ];	// Title Bar Text
DWORD ScreenWidth;		// Window Width
DWORD ScreenHeight;		// Window Height
DWORD ScreenColor;		// Number of colors (8, 16, 24, 32)
DWORD ScreenMode;		// Screen Mode
int ActiveWindow;		// Is the window currently active?
int SmallWindow;		// Is the window currently minimized?
RECT WindowPosition;	// Window Position
HMENU Menu;
DWORD WindowStyle;
// Full Screen
LONG Style = WS_VISIBLE;
int DestX = 0;
int DestY = 0;

//-------------------- Local Functions
int APIENTRY WinMain( HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show );
int InitWindow( HINSTANCE instance, int cmd_show, int width, int height );
LRESULT CALLBACK WndProc( HWND wnd, UINT message, WPARAM wparam, LPARAM lparam );
void SetWindowSize( int width, int height );
void ResetMenuCheck( int type );
void InitSetting( void );
LRESULT CALLBACK About( HWND dlg, UINT message, WPARAM wparam, LPARAM lparam );
void AnlyzeOption( char *cmd_line );
int GetKeyData( char *pOption, char *pKey );
int GetNumeral10( char *pData );
void PrintErrorMessage( void );

//============================================================================
//  WinMain Function
//============================================================================
int APIENTRY WinMain( HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show )
{
	DEBUGFILECLEAR();
	MSG msg;
	HACCEL accel_table;
//	DWORD time;
//	DWORD old_time;
//	DWORD diff_time;
    LONGLONG HTimerFreq;
    LARGE_INTEGER HTimerVal;
    LONGLONG LastTickCount;
	float LastT;
	int sum_time = 0;
	int frame_count = 0;
	// Variables Initialization
	ActiveWindow = 0;
	SmallWindow = 0;
	// Option
//	char *option_test = "/WIDTH=640 /HEIGHT=480";
//	cmd_line = option_test;	// Debug option string registration
	AnlyzeOption( (char*)cmd_line );
	// Initializes a global string
	LoadString( instance, IDS_APP_TITLE, Title, MAX_LOADSTRING );
	LoadString( instance, IDC_KEY, WindowClass, MAX_LOADSTRING );
	// Window Initialization
	DEBUGOUT( "****Application Start\n" );
	if( 0 == InitWindow( instance, cmd_show, ScreenWidth, ScreenHeight ) )
	{
		DEBUGOUT( "Window initialization failed.\n" );
		return 0;
	}
	// Loads the keyboard accelerator table
	accel_table = LoadAccelerators( instance, (LPCTSTR) IDC_KEY );
	// Initializing the COM library
	if( FAILED( CoInitialize( NULL ) ) )
	{
		DEBUGOUT( "COM library initialization failed.\n" );
		return 0;
	}
	if( false == InitSystem( Wnd ) )
	{
		DEBUGOUT( "Failed to initialize system library.\n" );
		return 0;
	}
	InitMain();
	InitSetting();
    
    QueryPerformanceFrequency(&HTimerVal);
    HTimerFreq = HTimerVal.QuadPart;
    QueryPerformanceCounter(&HTimerVal);
    LastTickCount = HTimerVal.QuadPart;

//    old_time = timeGetTime();
	for( ; ; )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			if( FALSE == GetMessage( &msg, NULL, 0, 0 ) )
			{
				break;
			}
			if( FALSE == TranslateAccelerator( msg.hwnd, accel_table, &msg ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		else
		{
			if( 0 == SmallWindow )
			{

                QueryPerformanceCounter(&HTimerVal);
                LastT = ((float) HTimerVal.QuadPart - LastTickCount) / 1000;
                //time = timeGetTime();
				//diff_time = time - old_time;
                // 200 = PAL
                float Latency = 200.0f;
                if(GetMachine() == false)
                {
                    Latency = 167.0f;
                }
				if( LastT > Latency )
				{
					InvalidateRect( Wnd, NULL, FALSE );
					UpdateWindow( Wnd );
					if( Main() )
					{
						DestroyWindow( Wnd );
					}
				    LastTickCount = HTimerVal.QuadPart;
					//old_time = time;
					sum_time += (int) LastT;
					frame_count ++;
				}
				if( sum_time > 1000 )
				{
					float fps;
					fps =  ( 10000.0f * ( float )frame_count ) / ( float )sum_time;
					SetFps( fps );
					frame_count = 0;
					sum_time = 0;
				}
				Sleep( 0 );
			}
		}
	}
	// End of Program
	ReleaseMain();
	ReleaseSystem();
	// Closing COM Library
	CoUninitialize();
	return (int)msg.wParam;
}

//============================================================================
//	Initialize Windows
//----------------------------------------------------------------------------
// In  : width = horizontal size
//     : height = vertical size
// Out : 1 = success / 0 = failure
//============================================================================
int InitWindow( HINSTANCE instance, int cmd_show, int width, int height )
{
	WNDCLASSEX wcex;
	int window_width;
	int window_height;
	// Save the instance handle in a global variable
	Instance = instance;
	// Window class definition
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= instance;
	wcex.hIcon			= LoadIcon( instance, (LPCTSTR) IDI_ICON );
	wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName	= (LPCSTR) IDC_MENU;
	wcex.lpszClassName	= WindowClass;
	wcex.hIconSm		= LoadIcon( wcex.hInstance, (LPCTSTR)IDI_SMALL );
	RegisterClassEx( &wcex );
	// Window Styling
	WindowStyle = WS_OVERLAPPEDWINDOW;
	// Adjust the window size
    window_width = width;
    window_height = height;
	window_width += ( GetSystemMetrics( SM_CXSIZEFRAME ) * 2 );
	window_height += ( GetSystemMetrics( SM_CYSIZEFRAME ) * 2 + GetSystemMetrics( SM_CYMENU ) + GetSystemMetrics( SM_CYCAPTION ) );
	// Window Creation
	Wnd = CreateWindow( WindowClass,
						Title,
						WindowStyle,
						CW_USEDEFAULT, CW_USEDEFAULT,
						window_width, window_height,
						NULL, NULL, instance, NULL );
	if( !Wnd )
	{
		return 0;
	}
	ShowWindow( Wnd, cmd_show );
	UpdateWindow( Wnd );
	// Save Menu Handles
	Menu = GetMenu( Wnd );
	// Save window coordinates
	GetWindowRect( Wnd, &WindowPosition );
    return 1;
}

void SetWindow( bool fullscreen )
{
	if( fullscreen && !GetFullScreen() )
	{
		// Go full screen
		DEVMODE devmode;
		HDC hdc = GetDC( NULL );
		ZeroMemory( &devmode, sizeof( devmode ) );
		devmode.dmSize = sizeof( devmode );
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		devmode.dmBitsPerPel = GetDeviceCaps( hdc, BITSPIXEL );
		devmode.dmPelsWidth = 320;
		devmode.dmPelsHeight = 240;
		ReleaseDC( NULL, hdc );
		if( DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettings( &devmode, CDS_TEST ) )
		{
			WINDOWPLACEMENT place;
			SetMenu( Wnd, NULL );
			place.length = sizeof( WINDOWPLACEMENT );
			GetWindowPlacement( Wnd, &place );
			DestX = place.rcNormalPosition.left;
			DestY = place.rcNormalPosition.top;
			ChangeDisplaySettings( &devmode, CDS_FULLSCREEN );
			Style = GetWindowLong( Wnd, GWL_STYLE );
			SetWindowLong( Wnd, GWL_STYLE, WS_VISIBLE );
			SetWindowPos( Wnd, HWND_TOP, 0, 0, 320, 240, SWP_SHOWWINDOW );
			SetCursorPos( 320, 240 );
			SetFullScreen( true );
		}
	}
	else if( !fullscreen && GetFullScreen() )
	{
		// Return from full screen
		RECT rect =
		{
			0, 0, 0, 0
		};
		rect.right = ScreenWidth;
		rect.bottom = ScreenHeight;
		SetMenu( Wnd, Menu );
		AdjustWindowRect( &rect, WindowStyle, TRUE );
		ChangeDisplaySettings( NULL, 0 );
		SetWindowLong( Wnd, GWL_STYLE, Style );
		SetWindowPos( Wnd, HWND_BOTTOM, DestX, DestY, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW );
		SetFullScreen( false );
	}
}

//============================================================================
//	Main Window Message Handling
//----------------------------------------------------------------------------
// In  :
// Out :
//============================================================================
LRESULT CALLBACK WndProc( HWND wnd, UINT message, WPARAM wparam, LPARAM lparam )
{
	int wm_id;
	int wm_event;
	switch( message )
	{
		case WM_ACTIVATEAPP:
			if( ( WA_ACTIVE == wparam ) || ( WA_CLICKACTIVE == wparam ) )
			{
				ActiveWindow = 1;
				SmallWindow = 0;
				InvalidateRect( wnd, NULL, FALSE );
				UpdateWindow( wnd );
			}
			else
			{
				ActiveWindow = 0;
			}
			break;
		case WM_SETCURSOR:
			if ( GetFullScreen() )
			{
				SetCursor( NULL );
			}
			break;
		case WM_COMMAND:
			wm_id = LOWORD( wparam );
			wm_event = HIWORD( wparam );
			// Parsing menu selections
			switch( wm_id )
			{
				case ID_SET_READTAPE:
					SetReadTape();
					break;
				case ID_REW_READTAPE:
					RewReadTape();
					break;
				case ID_SET_WRITETAPE:
					SetWriteTape();
					break;
				case ID_RESET:
					Reset( false );
					break;
				case ID_RESET_CLEAR:
					Reset( true );
					break;
				case IDM_MONITOR:
                    if(Monitor_Is_Opened())
                    {
                        Monitor_Close();
                    }
                    else
                    {
                        Monitor_Open();
                    }
					break;
				case IDM_EXIT:
					DestroyWindow( wnd );
					break;
				case ID_WINDOWSIZEx1:
					ResetMenuCheck( MENU_SCREENSIZE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetWindow( false );
					SetWindowSize( ScreenWidth, ScreenHeight );
					SetScreenScale( 1 );
					break;
				case ID_WINDOWSIZEx2:
					ResetMenuCheck( MENU_SCREENSIZE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetWindow( false );
					SetWindowSize( ScreenWidth * 2, ScreenHeight * 2 );
					SetScreenScale( 2 );
					break;
				case ID_WINDOWSIZEx3:
					ResetMenuCheck( MENU_SCREENSIZE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetWindow( false );
					SetWindowSize( ScreenWidth * 3, ScreenHeight * 3 );
					SetScreenScale( 3 );
					break;
				case ID_WINDOWSIZEx4:
					ResetMenuCheck( MENU_SCREENSIZE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetWindow( false );
					SetWindowSize( ScreenWidth * 4, ScreenHeight * 4 );
					SetScreenScale( 4 );
					break;
				case ID_FULLSCREEN:
				case IDM_CHANGESCREEN:
					if( GetFullScreen() )
					{
						ResetMenuCheck( MENU_SCREENSIZE );
						SetWindow( false );
						SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
						SetFullScreen( false );
						switch( GetScreenScale() )
						{
							case 1:
								CheckMenuItem( Menu, ID_WINDOWSIZEx1, MF_BYCOMMAND | MF_CHECKED );
								break;
							case 2:
								CheckMenuItem( Menu, ID_WINDOWSIZEx2, MF_BYCOMMAND | MF_CHECKED );
								break;
							case 3:
								CheckMenuItem( Menu, ID_WINDOWSIZEx3, MF_BYCOMMAND | MF_CHECKED );
								break;
							case 4:
								CheckMenuItem( Menu, ID_WINDOWSIZEx4, MF_BYCOMMAND | MF_CHECKED );
								break;
						}
					}
					else
					{
						ResetMenuCheck( MENU_SCREENSIZE );
						SetWindow( true );
						CheckMenuItem( Menu, ID_FULLSCREEN, MF_BYCOMMAND | MF_CHECKED );
						SetFullScreen( true );
					}
					break;
				case ID_MACHINE_PAL:
					SetMachine( true );
					ResetMenuCheck( MENU_MACHINE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					break;
				case ID_MACHINE_NTSC:
					SetMachine( false );
					ResetMenuCheck( MENU_MACHINE );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					break;
				case ID_DRAW_STATUS:
					if( GetFullScreen() )
					{
						if( GetInformation() )
						{
							CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_UNCHECKED );
							SetInformation( false );
							ScreenHeight = 192;
						}
						else
						{
							CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_CHECKED );
							SetInformation( true );
							ScreenHeight = 192 + 12;
						}
					}
					else
					{
						if( GetInformation() )
						{
							CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_UNCHECKED );
							SetInformation( false );
							ScreenHeight = 192;
							SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
						}
						else
						{
							CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_CHECKED );
							SetInformation( true );
							ScreenHeight = 192 + 12;
							SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
						}
					}
					break;
				case ID_DRAW_TV:
					if( GetTV() )
					{
						CheckMenuItem( Menu, ID_DRAW_TV, MF_BYCOMMAND | MF_UNCHECKED );
						SetTV( false );
					}
					else
					{
						CheckMenuItem( Menu, ID_DRAW_TV, MF_BYCOMMAND | MF_CHECKED );
						SetTV( true );
					}
					break;
				case ID_SOUND_VOL100:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 100 );
					break;
				case ID_SOUND_VOL90:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 90 );
					break;
				case ID_SOUND_VOL80:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 80 );
					break;
				case ID_SOUND_VOL70:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 70 );
					break;
				case ID_SOUND_VOL60:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 60 );
					break;
				case ID_SOUND_VOL50:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 50 );
					break;
				case ID_SOUND_VOL40:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 40 );
					break;
				case ID_SOUND_VOL30:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 30 );
					break;
				case ID_SOUND_VOL20:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 20 );
					break;
				case ID_SOUND_VOL10:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 10 );
					break;
				case ID_SOUND_MUTE:
					ResetMenuCheck( MENU_VOLUME );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetVolume( 0 );
					break;
				case ID_SOUND_BUF10:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 0 );
					break;
				case ID_SOUND_BUF15:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 1 );
					break;
				case ID_SOUND_BUF20:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 2 );
					break;
				case ID_SOUND_BUF25:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 3 );
					break;
				case ID_SOUND_BUF30:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 4 );
					break;
				case ID_SOUND_BUF35:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 5 );
					break;
				case ID_SOUND_BUF40:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 6 );
					break;
				case ID_SOUND_BUF45:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 7 );
					break;
				case ID_SOUND_BUF50:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 8 );
					break;
				case ID_SOUND_BUF75:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 9 );
					break;
				case ID_SOUND_BUF100:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 10 );
					break;
				case ID_SOUND_BUF125:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 11 );
					break;
				case ID_SOUND_BUF150:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 12 );
					break;
				case ID_SOUND_BUF200:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 13 );
					break;
				case ID_SOUND_BUF500:
					ResetMenuCheck( MENU_SOUNDBUFFER );
					CheckMenuItem( Menu, wm_id, MF_BYCOMMAND | MF_CHECKED );
					SetSound( 44100, 14 );
					break;
				case ID_SETPRINTER:
					SetPrinter();
					break;
				case ID_WARPMODE:
					if( GetWarpMode() )
					{
						CheckMenuItem( Menu, ID_WARPMODE, MF_BYCOMMAND | MF_UNCHECKED );
						SetWarpMode( false );
					}
					else
					{
						CheckMenuItem( Menu, ID_WARPMODE, MF_BYCOMMAND | MF_CHECKED );
						SetWarpMode( true );
					}
					break;
				case IDM_ABOUT:
					DialogBox( Instance, (LPCTSTR)IDD_ABOUTBOX, wnd, (DLGPROC)About );
					break;
				default:
				   return DefWindowProc( wnd, message, wparam, lparam );
			}
			break;
		case WM_DESTROY:
			// A trick to prevent it from remaining on the taskbar
			ShowWindow( wnd, SW_HIDE );
            Monitor_Close();
			Sleep( 500 );
			// End of Program
			PostQuitMessage( 0 );
			break;
		case WM_KEYDOWN:
			SetKey( ( int )wparam );
			break;
		case WM_KEYUP:
			ResetKey( ( int )wparam );
			break;
		case WM_PAINT:
			Render( GetFullScreen() );
			break;
		case WM_SIZE:
			if( SIZE_MAXIMIZED == wparam )
			{
				// Window mode change
			}
			if( SIZE_MINIMIZED == wparam )
			{
				SmallWindow = 1;
			}
			if( ( SIZE_MAXHIDE == wparam ) || ( SIZE_MINIMIZED == wparam ) )
			{
				ActiveWindow = 0;
			}
			else
			{
				ActiveWindow = 1;
				SmallWindow = 0;
			}
			break;
	}
	return DefWindowProc( wnd, message, wparam, lparam );
}

//============================================================================
//	Window resize
//============================================================================
void SetWindowSize( int width, int height )
{
	RECT position;
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	// Window Position
	GetWindowRect( Wnd, &position );
	// Window Size
	AdjustWindowRect( &rect, WindowStyle, TRUE );
	if( rect.left != 0 )
	{
		rect.right -= rect.left;
		rect.left = 0;
	}
	if( rect.top != 0 )
	{
		rect.bottom -= rect.top;
		rect.top = 0;
	}
	SetWindowPos( Wnd, HWND_TOP,
					position.left, position.top,
					rect.right,
					rect.bottom,
					SWP_SHOWWINDOW );
}

void ResetMenuCheck( int type )
{
	if( MENU_SCREENSIZE == type )
	{
		// Screen
		CheckMenuItem( Menu, ID_WINDOWSIZEx1, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_WINDOWSIZEx2, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_WINDOWSIZEx3, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_WINDOWSIZEx4, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_FULLSCREEN, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else if( MENU_MACHINE == type )
	{
		// Frame skip
		CheckMenuItem( Menu, ID_MACHINE_PAL, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_MACHINE_NTSC, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else if( MENU_VOLUME == type )
	{
		// Volume
		CheckMenuItem( Menu, ID_SOUND_VOL100, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL90, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL80, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL70, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL60, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL50, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL40, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL30, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL20, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_VOL10, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_MUTE, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else if( MENU_SOUNDBUFFER == type )
	{
		// Sound buffer
		CheckMenuItem( Menu, ID_SOUND_BUF10, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF15, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF20, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF25, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF30, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF35, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF40, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF45, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF50, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF75, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF100, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF125, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF150, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF200, MF_BYCOMMAND | MF_UNCHECKED );
		CheckMenuItem( Menu, ID_SOUND_BUF500, MF_BYCOMMAND | MF_UNCHECKED );
	}
}

void InitSetting( void )
{
	// Screen
	if( GetFullScreen() )
	{
		ResetMenuCheck( MENU_SCREENSIZE );
		SetFullScreen( false );
		SetWindow( true );
		SetFullScreen( true );
		CheckMenuItem( Menu, ID_FULLSCREEN, MF_BYCOMMAND | MF_CHECKED );
	}
	else
	{
		ResetMenuCheck( MENU_SCREENSIZE );
		SetWindow( false );
		SetFullScreen( false );
		SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
		switch( GetScreenScale() )
		{
			case 1:
				CheckMenuItem( Menu, ID_WINDOWSIZEx1, MF_BYCOMMAND | MF_CHECKED );
				break;
			case 2:
				CheckMenuItem( Menu, ID_WINDOWSIZEx2, MF_BYCOMMAND | MF_CHECKED );
				break;
			case 3:
				CheckMenuItem( Menu, ID_WINDOWSIZEx3, MF_BYCOMMAND | MF_CHECKED );
				break;
			case 4:
				CheckMenuItem( Menu, ID_WINDOWSIZEx4, MF_BYCOMMAND | MF_CHECKED );
				break;
		}
	}
	ResetMenuCheck( MENU_MACHINE );
	if( GetMachine() )
	{
		CheckMenuItem( Menu, ID_MACHINE_PAL, MF_BYCOMMAND | MF_CHECKED );
	}
	else
	{
		CheckMenuItem( Menu, ID_MACHINE_NTSC, MF_BYCOMMAND | MF_CHECKED );
	}
	ResetMenuCheck( MENU_VOLUME );
	if( GetVolume() < 10 )
	{
		CheckMenuItem( Menu, ID_SOUND_MUTE, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 20 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL10, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 30 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL20, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 40 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL30, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 50 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL40, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 60 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL50, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 70 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL60, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 80 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL70, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 90 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL80, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() < 100 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL90, MF_BYCOMMAND | MF_CHECKED );
	}
	else if( GetVolume() >= 100 )
	{
		CheckMenuItem( Menu, ID_SOUND_VOL100, MF_BYCOMMAND | MF_CHECKED );
	}
	ResetMenuCheck( MENU_SOUNDBUFFER );
	switch( GetSoundBuffer() )
	{
		case 0:
			SetSound( 44100, 0 );
			CheckMenuItem( Menu, ID_SOUND_BUF10, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 1:
			SetSound( 44100, 1 );
			CheckMenuItem( Menu, ID_SOUND_BUF15, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 2:
			SetSound( 44100, 2 );
			CheckMenuItem( Menu, ID_SOUND_BUF20, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 3:
			SetSound( 44100, 3 );
			CheckMenuItem( Menu, ID_SOUND_BUF25, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 4:
			SetSound( 44100, 4 );
			CheckMenuItem( Menu, ID_SOUND_BUF30, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 5:
			SetSound( 44100, 5 );
			CheckMenuItem( Menu, ID_SOUND_BUF35, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 6:
			SetSound( 44100, 6 );
			CheckMenuItem( Menu, ID_SOUND_BUF40, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 7:
			SetSound( 44100, 7 );
			CheckMenuItem( Menu, ID_SOUND_BUF45, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 8:
			SetSound( 44100, 8 );
			CheckMenuItem( Menu, ID_SOUND_BUF50, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 9:
			SetSound( 44100, 9 );
			CheckMenuItem( Menu, ID_SOUND_BUF75, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 10:
			SetSound( 44100, 10 );
			CheckMenuItem( Menu, ID_SOUND_BUF100, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 11:
			SetSound( 44100, 11 );
			CheckMenuItem( Menu, ID_SOUND_BUF125, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 12:
			SetSound( 44100, 12 );
			CheckMenuItem( Menu, ID_SOUND_BUF150, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 13:
			SetSound( 44100, 13 );
			CheckMenuItem( Menu, ID_SOUND_BUF200, MF_BYCOMMAND | MF_CHECKED );
			break;
		case 14:
			SetSound( 44100, 14 );
			CheckMenuItem( Menu, ID_SOUND_BUF500, MF_BYCOMMAND | MF_CHECKED );
			break;
	}
	if( GetFullScreen() )
	{
		if( GetInformation() )
		{
			CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_CHECKED );
			SetInformation( true );
			ScreenHeight = 192 + 12;
		}
		else
		{
			CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_UNCHECKED );
			SetInformation( false );
			ScreenHeight = 192;
		}
	}
	else
	{
		if( GetInformation() )
		{
			CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_CHECKED );
			SetInformation( true );
			ScreenHeight = 192 + 12;
			SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
		}
		else
		{
			CheckMenuItem( Menu, ID_DRAW_STATUS, MF_BYCOMMAND | MF_UNCHECKED );
			SetInformation( false );
			ScreenHeight = 192;
			SetWindowSize( ScreenWidth * GetScreenScale(), ScreenHeight * GetScreenScale() );
		}
	}
	if( GetTV() )
	{
		CheckMenuItem( Menu, ID_DRAW_TV, MF_BYCOMMAND | MF_CHECKED );
		SetTV( true );
	}
	else
	{
		CheckMenuItem( Menu, ID_DRAW_TV, MF_BYCOMMAND | MF_UNCHECKED );
		SetTV( false );
	}
	if( GetWarpMode() )
	{
		CheckMenuItem( Menu, ID_WARPMODE, MF_BYCOMMAND | MF_CHECKED );
		SetWarpMode( true );
	}
	else
	{
		CheckMenuItem( Menu, ID_WARPMODE, MF_BYCOMMAND | MF_UNCHECKED );
		SetWarpMode( false );
	}
}

//============================================================================
//	Message handling for the About box
//============================================================================
LRESULT CALLBACK About( HWND dlg, UINT message, WPARAM wparam, LPARAM lparam )
{
	switch( message )
	{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			if( ( IDOK == LOWORD( wparam ) ) || ( IDCANCEL == LOWORD( wparam ) ) )
			{
				EndDialog( dlg, LOWORD( wparam ) );
				return TRUE;
			}
			break;
	}
    return FALSE;
}

//============================================================================
//	Command Line Parsing
//============================================================================
void AnlyzeOption( char *cmd_line )
{
	// Option Debug Display
	DEBUGOUT( "****Options\n" );
	DEBUGOUT( "%s\n", cmd_line );
	// Default Settings
	ScreenWidth = 256;
	ScreenHeight = 192 + 12;
	ScreenColor = 32;
	// Get options
	// Option Analysis
	DEBUGOUT( "****Option analysis results\n" );
	DEBUGOUT( "None\n" );
}

//============================================================================
//	Get numeric data of option string
//----------------------------------------------------------------------------
// In  : Option = pointer to option string
//     : Key = option key
// Out : Character position
//============================================================================
int GetKeyData( char *option, char *key )
{
	char key_temp[ 260 ];
	int option_length;
	int key_length;
	int loop;
	int key_position;
	int data_position;
	int ret_code;
	int i;
	strcpy( key_temp, "/" );
	strcat( key_temp, key );
	strcat( key_temp, "=" );
	option_length = (int)strlen( option );
	key_length = (int)strlen( key );
	loop = option_length - key_length;
	key_position = -1;
	for( i = 0; i < loop; i ++ )
	{
		if( 0 == strnicmp( &option[ i ], key_temp, key_length ) )
		{
			key_position = i;
			break;
		}
	}
	if( -1 == key_position )
	{
		return -1;
	}
	data_position = key_position + key_length + 2;
	ret_code = GetNumeral10( &option[ data_position ] );
	return ret_code;
}

//============================================================================
//  Get numeric data from a string
//----------------------------------------------------------------------------
// In  : String Data
// Out : Numerical Data
//============================================================================
int GetNumeral10( char *data )
{
	int ret_code = 0;
	while( !( ( *data > '9' ) || ( *data < '0' ) ) )
	{
		ret_code *= 10;
		ret_code += ( (int)*data - (int)'0' );
		data++;
	}
	return( ret_code );
}

void PrintErrorMessage( void )
{
	LPVOID message_buffer;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default Language
		(LPTSTR)&message_buffer,
		0,
		NULL 
	);
	MessageBox( NULL, (LPTSTR)message_buffer, "Error", MB_OK | MB_ICONINFORMATION );
	LocalFree( message_buffer );
}
