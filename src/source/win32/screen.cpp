#include <windows.h>
#include "screen.h"
#include "../common/debug.h"

#define ADJUST 12

ScreenClass::ScreenClass( void )
:Vram( 0 ), Font( 0 ), WindowHandle( NULL ), Infomation( true )
{
}

ScreenClass::~ScreenClass( void )
{
	if( WindowHandle )
	{
		Release();
	}
}

void ScreenClass::Create( HWND window_handle, void *vram, void *font )
{
	WindowHandle = window_handle;
	Vram = ( BYTE * )vram;
	Font = ( BYTE * )font;
	// CreateDIBSection Initialization
	HDC hdc;
	hdc = GetDC( window_handle );
	Buffer = (LPBYTE)GlobalAlloc( GPTR, sizeof( BITMAPINFO ) );
	BitmapInfo = (LPBITMAPINFO)Buffer;
	BitmapInfo->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	BitmapInfo->bmiHeader.biWidth = 256;
	BitmapInfo->bmiHeader.biHeight = 192 + 12;
	BitmapInfo->bmiHeader.biPlanes = 1;
	BitmapInfo->bmiHeader.biBitCount = 16;
	BitmapInfo->bmiHeader.biCompression = BI_RGB;
	BitmapInfo->bmiHeader.biSizeImage = 0;
	BitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	BitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	BitmapInfo->bmiHeader.biClrUsed = 0;
	BitmapInfo->bmiHeader.biClrImportant = 0;
	BitmapHandle = CreateDIBSection( hdc, BitmapInfo, DIB_RGB_COLORS, (PVOID*)&Bitmap, NULL, 0 );
	BitmapHDC = CreateCompatibleDC( hdc );
	SelectObject( BitmapHDC, BitmapHandle );
	// Default Text Color
	TextColor[ 0 ] = LIGHTGREEN;
	TextColor[ 1 ] = GREEN;
	TextColor[ 2 ] = BEIGE;
	TextColor[ 3 ] = RED;
	TextColor[ 4 ] = GREEN;
	TextColor[ 5 ] = BLACK;
	// Default Semi-Graphic Color
	SemiGraphColor[ 0 ] = LIGHTGREEN;
	SemiGraphColor[ 1 ] = YELLOW;
	SemiGraphColor[ 2 ] = BLUE;
	SemiGraphColor[ 3 ] = RED;
	SemiGraphColor[ 4 ] = WHITE;
	SemiGraphColor[ 5 ] = CYANOGEN;
	SemiGraphColor[ 6 ] = MAGENTA;
	SemiGraphColor[ 7 ] = ORANGE;
	SemiGraphColor[ 8 ] = BLACK;
	// DEFAULT SCREEN 3 GRAPHICS COLOR
	Graph3Color[ 0 ] = GPAPH3_GREEN;
	Graph3Color[ 1 ] = GPAPH3_YELLOW;
	Graph3Color[ 2 ] = GPAPH3_BLUE;
	Graph3Color[ 3 ] = GPAPH3_RED;
	Graph3Color[ 4 ] = GPAPH3_WHITE;
	Graph3Color[ 5 ] = GPAPH3_CYANOGEN;
	Graph3Color[ 6 ] = GPAPH3_MAGENTA;
	Graph3Color[ 7 ] = GPAPH3_ORANGE;
	// Default SCREEN 1 graphic color
	Graph4Color[ 0 ] = GREEN;
	Graph4Color[ 1 ] = LIGHTGREEN;
	Graph4Color[ 2 ] = BLACK;
	Graph4Color[ 3 ] = WHITE;
	// SCREEN 4 Graphic Color
	// COLOR ,,1
	Graph4Color[ 4 ] = GREEN;
	Graph4Color[ 5 ] = GRAPH4_1_1;
	Graph4Color[ 6 ] = GRAPH4_1_2;
	Graph4Color[ 7 ] = LIGHTGREEN;
	// COLOR ,,2
	Graph4Color[ 8 ] = BLACK;
	Graph4Color[ 9 ] = GRAPH4_2_1;
	Graph4Color[ 10 ] = GRAPH4_2_2;
	Graph4Color[ 11 ] = WHITE;
	// Special characters
	BYTE ext_font[ 12 * 3 ] =
	{
		0x00, 0xE0, 0x90, 0x90, 0xE0, 0xA0, 0x9C, 0x0A, 0x09, 0x09, 0x0A, 0x0C,
		0x00, 0x88, 0xA8, 0xA8, 0xA8, 0x50, 0x56, 0x09, 0x09, 0x0E, 0x0A, 0x09,
		0xF0, 0x80, 0xE0, 0x80, 0xB8, 0x24, 0x38, 0x27, 0x08, 0x06, 0x01, 0x0E
	};
	memcpy( &ExtFont, ext_font, sizeof( ExtFont ) );
	Infomation = true;
	Blot = true;
}

void ScreenClass::Release( void )
{
	GlobalFree( Buffer );
	DeleteDC( BitmapHDC );
	DeleteObject( BitmapHandle );
	WindowHandle = 0;
	Vram = 0;
	Font = 0;
}

void ScreenClass::Draw( HDC hdc, bool fullscreen )
{
	RECT dest_rect;
	int adjust = 0;
	if( Infomation )
	{
		adjust = ADJUST;
	}
	GetClientRect( WindowHandle, &dest_rect );
	if( fullscreen )
	{
		int x;
		int y;
		x = ( ( dest_rect.right - dest_rect.left ) - 256 ) / 2;
		y = ( ( dest_rect.bottom - dest_rect.top ) - ( 192 + adjust ) ) / 2;
		// Erase off-screen
		HPEN pen;
		HBRUSH brush;
		pen = CreatePen( PS_SOLID, 0, RGB( 0, 0, 0 ) );
		brush = CreateSolidBrush( RGB( 0, 0, 0 ) );
		SelectObject( hdc, pen );
		SelectObject( hdc, brush );
		Rectangle( hdc, 0, 0, x, 240 );
		Rectangle( hdc, x + 256, 0, x * 2 + 256, 240 );
		Rectangle( hdc, x, 0, x * 2 + 256, y );
		Rectangle( hdc, x, y + 192 + adjust, x + 256, y * 2 + 192 + adjust );
		DeleteObject( pen );
		DeleteObject( brush );
		// Screen drawing
		BitBlt( hdc, x, y, 256, 192 + adjust, BitmapHDC, 0, 0, SRCCOPY );
	}
	else
	{
		if( ( 256 == dest_rect.right ) && ( ( 192 + adjust ) == dest_rect.bottom ) )
		{
			BitBlt( hdc, 0, 0, 256, 192 + adjust, BitmapHDC, 0, 0, SRCCOPY );
		}
		else
		{
			StretchBlt( hdc, dest_rect.left, dest_rect.top, dest_rect.right, dest_rect.bottom, BitmapHDC, 0, 0, 256, 192 + adjust, SRCCOPY );
		}
	}
}

void ScreenClass::Render( int screen_mode, unsigned char *vram, int select_color )
{
	int y;
	int x;
	int i = 0;
	switch( screen_mode )
	{
		case MODE_TEXT:
		{
			for( y = 0; y < 16; y ++ )
			{
				for( x = 0; x < 32; x ++ )
				{
					DrawFont( x, y, vram[ i ], vram[ 0x800 + i ] );
					i ++;
				}
			}
			break;
		}
		case MODE_GRAPH3:
		{
			for( y = 0; y < 192; y ++ )
			{
				for( x = 0; x < 32; x ++ )
				{
					DrawGraph3( x, y, vram[ i ], select_color );
					i ++;
				}
			}
			break;
		}
		case MODE_GRAPH4:
		{
			for( y = 0; y < 192; y ++ )
			{
				for( x = 0; x < 32; x ++ )
				{
					DrawGraph4( x, y, vram[ i ], select_color );
					i ++;
				}
			}
			break;
		}
	}
}

void ScreenClass::SetTextColor( int index, WORD color )
{
	TextColor[ index ] = color;
}

void ScreenClass::SetColor3( int index, WORD color1, WORD color2, WORD color3, WORD color4 )
{
	Graph3Color[ index * 4 ] = color1;
	Graph3Color[ index * 4 + 1 ] = color2;
	Graph3Color[ index * 4 + 2 ] = color3;
	Graph3Color[ index * 4 + 3 ] = color4;
}

void ScreenClass::SetColor4( int index, WORD color1, WORD color2 )
{
	Graph4Color[ index * 2 ] = color1;
	Graph4Color[ index * 2 + 1 ] = color2;
}

void ScreenClass::DrawFont( int x, int y, int chr, int attr )
{
	int i;
	int j;
	if( ( attr & 2 ) && ( attr > 0 ) )
	{
		// Semi-Graphic
		const int position_table[ 6 ] =
		{
			-256 * 8 + 4, -256 * 8,
			-256 * 4 + 4, -256 * 4,
			4, 0,
		};
		int color;
		int box_x;
		int box_y;
		color = ( attr & 4 ) + ( chr >> 6 );
		for( i = 0; i < 6; i ++ )
		{
			int draw_color = 8;
			int draw_address;
			if( chr & 1 )
			{
				draw_color = color;
			}
			for( box_y = 0; box_y < 4; box_y ++ )
			{
				for( box_x = 0; box_x < 4; box_x ++ )
				{
					draw_address = ( ( 191 + ADJUST ) - y * 12 ) * 256 + ( x * 8 );
					draw_address += position_table[ i ];
					Bitmap[ draw_address - box_y * 256 + box_x ] = SemiGraphColor[ draw_color ];
				}
			}
			chr >>= 1;
		}
	}
	else
	{
		// Text
		if( -1 == attr )
		{
			attr = 4;
		}
		else
		{
			attr = ( attr & 1 ) + ( ( attr & 4 ) >> 1 );
		}
		for( i = 0; i < 12; i ++ )
		{
			BYTE pattern;
			int draw_address;
			if( chr < 256 )
			{
				pattern = *( BYTE * )( Font + ((chr -2) * 12) + i + 4 );
			}
			else
			{
				pattern = *( BYTE * )( ExtFont + ( chr - 256 ) * 12 + i );
			}
			draw_address = ( ( 191 + ADJUST ) - ( y * 12 + i ) ) * 256 + ( x * 8 );
			for( j = 7; j >= 0; j -- )
			{
				if( pattern & 1 )
				{
					Bitmap[ draw_address + j ] = TextColor[ attr ];
				}
				else
				{
					Bitmap[ draw_address + j ] = TextColor[ attr ^ 1 ];
				}
				pattern >>= 1;
			}
		}
	}
}

void ScreenClass::DrawGraph3( int x, int y, int chr, int select_color )
{
	int draw_address;
	int i;
	draw_address = ( ( 191 + ADJUST ) - y ) * 256 + x * 8 + 6;
	for( i = 0; i < 4; i ++ )
	{
		int color;
		color = chr & 3;
		Bitmap[ draw_address ] = Graph3Color[ select_color * 4 + color ];
		Bitmap[ draw_address + 1 ] = Graph3Color[ select_color * 4 + color ];
		draw_address -= 2;
		chr >>= 2;
	}
}

void ScreenClass::DrawGraph4( int x, int y, int chr, int select_color )
{
	int draw_address;
	int i;
	draw_address = ( ( 191 + ADJUST ) - y ) * 256 + x * 8 + 7;
	if( Blot )
	{
		for( i = 0; i < 4; i ++ )
		{
			int color;
			color = chr & 3;
			Bitmap[ draw_address -- ] = Graph4Color[ 4 + select_color * 4 + color ];
			Bitmap[ draw_address -- ] = Graph4Color[ 4 + select_color * 4 + color ];
			chr >>= 2;
		}
	}
	else
	{
		for( i = 0; i < 8; i ++ )
		{
			int color;
			color = chr & 1;
			Bitmap[ draw_address ] = Graph4Color[ select_color * 2 + color ];
			draw_address --;
			chr >>= 1;
		}
	}
}

void ScreenClass::SetInfomation( bool information )
{
	Infomation = information;
}

void ScreenClass::SetBlot( bool blot )
{
	Blot = blot;
}
