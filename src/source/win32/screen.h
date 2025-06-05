//============================================================================
//  screen.h "Win32 Screen Control"
//													Programmed by Keiichi Tago
//----------------------------------------------------------------------------
// Creation started on 2004.04.09.
//============================================================================

// -------------------- Double definition prevention
#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>

#define RGB16(r,g,b) (((r>>3)<<10)+((g>>3)<<5)+(b>>3)) // R5G5B5
#define MODE_TEXT   0
#define MODE_GRAPH3 1
#define MODE_GRAPH4 2

#define GREEN      RGB16(22,134,10)
#define LIGHTGREEN RGB16(184,255,181)
#define RED        RGB16(254,65,105)
#define BEIGE      RGB16(255,198,170)
#define YELLOW     RGB16(252,253,153)
#define BLUE       RGB16(116,41,255)
#define WHITE      RGB16(241,229,232)
#define CYANOGEN   RGB16(124,210,213)
#define MAGENTA    RGB16(254,113,255)
#define ORANGE     RGB16(254,112,35)
#define BLACK      RGB16(0,0,0)

#define GPAPH3_GREEN    RGB16(0,255,0)
#define GPAPH3_YELLOW   RGB16(255,255,0)
#define GPAPH3_BLUE     RGB16(0,0,255)
#define GPAPH3_RED      RGB16(255,0,0)
#define GPAPH3_WHITE    RGB16(241,229,232)
#define GPAPH3_CYANOGEN RGB16(255,0,255)
#define GPAPH3_MAGENTA  RGB16(0,255,255)
#define GPAPH3_ORANGE   RGB16(255,174,0)
#define GPAPH3_BLACK    RGB16(0,0,0)

#define GRAPH4_1_1     RGB16(235,141,255)
#define GRAPH4_1_2     RGB16(220,255,181)
#define GRAPH4_2_1     RGB16(255,66,35)
#define GRAPH4_2_2     RGB16(21,120,255)

class ScreenClass
{
	public:
		ScreenClass( void );
		~ScreenClass( void );
		void Create( HWND window_handle, void *vram, void *font );
		void Release();
		void Draw( HDC hdc, bool fullscreen );
		void Render( int screen_mode, unsigned char *vram, int select_color = 0 );
		void DrawFont( int x, int y, int chr, int attr );
		void SetInfomation( bool information );
		void SetBlot( bool blot );
	private:
		BYTE *Vram;
		BYTE *Font;
		BYTE ExtFont[ 12 * 3 ];		// Special characters x3
		// Window
		HWND WindowHandle;
		// CreateDIBSection
		HDC BitmapHDC;				//hdcDIB;
		HBITMAP BitmapHandle;		//hBMP;
		LPBYTE Buffer;				//lpBuf;
		LPWORD Bitmap;				//lpBMP;
		LPBITMAPINFO BitmapInfo;	//lpDIB;
		// Color
		WORD TextColor[ 6 ];
		WORD SemiGraphColor[ 9 ];
		WORD Graph3Color[ 8 ];
		WORD Graph4Color[ 12 ];
		// Smear
		bool Blot;
		// Size
		bool Infomation;
		// Function
		void SetTextColor( int index, WORD color );
		void SetColor3( int index, WORD color1, WORD color2, WORD color3, WORD color4 );
		void SetColor4( int index, WORD color1, WORD color2 );
		void DrawGraph3( int x, int y, int chr, int select_color );
		void DrawGraph4( int x, int y, int chr, int select_color );
};

#endif
