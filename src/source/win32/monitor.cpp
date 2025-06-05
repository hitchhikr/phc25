// Rudimentary monitor
#include <stdio.h>
#include <windows.h>
#include <stdarg.h>
#include "resource.h"
#include "monitor.h"
#include "../common/z80.h"
#include "../common/z80diag.h"

static int Monitor_Opened = FALSE;
static HWND hWnd;
static HWND hTextBox;
extern HINSTANCE Instance;
static HFONT hFont;
static HBITMAP hbitmap;
static int FontHeight;
char Buf[31][80];
char NewEntry[256];
char TempEntry[256];
int Breakpoints[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int Z80_Pause = FALSE;
int Z80_Step = FALSE;
int pos = 0;
int display_address = 0;
extern Z80 z80;
char HEXTabASCII[] =
{
    "000102030405060708090A0B0C0D0E0F"
    "101112131415161718191A1B1C1D1E1F"
    "202122232425262728292A2B2C2D2E2F"
    "303132333435363738393A3B3C3D3E3F"
    "404142434445464748494A4B4C4D4E4F"
    "505152535455565758595A5B5C5D5E5F"
    "606162636465666768696A6B6C6D6E6F"
    "707172737475767778797A7B7C7D7E7F"
    "808182838485868788898A8B8C8D8E8F"
    "909192939495969798999A9B9C9D9E9F"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
    "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
    "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
    "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
    "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
    "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
};

void Push_Text(char *Format, ...)
{
    int i;
    char szBuffer[256];
    va_list Arg;

    va_start(Arg, Format);
        vsprintf(szBuffer, Format, Arg);
    va_end(Arg);

    for(i = 1; i < 31; i++)
    {
        memcpy(Buf[i - 1], Buf[i], 80);
    }
    memset(Buf[30], ' ', 80);
    strncpy(Buf[30], szBuffer, strlen(szBuffer));

}

int Get_Address(char *Cmd, char **pEnd)
{
    int i = 0;
    int Value_Hex = FALSE;
    int value;

    // Pass over
    while(Cmd[i] == ' ' || Cmd[i] == '\t')
    {
        i++;
    }
    if(Cmd[i] == 'P' && Cmd[i + 1] == 'C')
    {
        *pEnd = &Cmd[i + 2];
        return z80.PC;
    }
    if(Cmd[i] == '$' || Cmd[i] == '#')
    {
        i++;
        Value_Hex = TRUE;
    }
    value = strtol(&Cmd[i], pEnd, Value_Hex ? 16 : 10);
    return (value & 0xffff);
}

int Dis_Line(int pc)
{
    int i;
    int k;
    int old_pc;
    char dis_line[256];
    char hex_line[256];
    unsigned char mem_value;

    old_pc = pc;
    memset(dis_line, 0, sizeof(dis_line));
    pc = Disassemble(pc, dis_line) & 0xffff;

    k = 0;
    for(i = 0; i < (pc - old_pc); i++)
    {
        mem_value = z80.ReadMemory(old_pc + i);
        hex_line[k] = HEXTabASCII[(mem_value * 2)];
        hex_line[k + 1] = HEXTabASCII[(mem_value * 2) + 1];
        hex_line[k + 2] = ' ';
        k += 3;
    }
    for(; k < 16; k++)
    {
        hex_line[k] = ' ';
    }
    hex_line[k] = 0;
    Push_Text("$%.04X: %s %s", old_pc, hex_line, dis_line);
    return pc;
}

void Print_Registers()
{
    Push_Text("PC: $%.04X SP: $%.04X", z80.PC, z80.SP);
    Push_Text("AF: $%.04X BC: $%.04X AF': $%.04X BC': $%.04X", z80.regs[0].w, z80.regs[1].w, z80.exAF, z80.exBC);
    Push_Text("HL: $%.04X DE: $%.04X HL': $%.04X DE': $%.04X", z80.regs[3].w, z80.regs[2].w, z80.exHL, z80.exDE);
    Push_Text("IX: $%.04X IY: $%.04X", z80.regs[4].w, z80.regs[5].w);
    Push_Text(" I: $%.02X   IM: %.01d", z80._I , z80.IM);
    Dis_Line(z80.PC);
}

void Parse_Command(char *Cmd)
{
    int i;
    int j;
    int k;
    int l;
    int bp_number;
    unsigned char mem_value;
    unsigned char mem_line[256];
    char *pEnd = 0;

    i = 0;
    while(Cmd[i] == ' ' || Cmd[i] == '\t')
    {
        i++;
    }

    switch(Cmd[i])
    {
        case 'H':
            Push_Text("");
            Push_Text("**AVAILABLE COMMANDS**");
            Push_Text("");
            Push_Text("H             : THIS PAGE");
            Push_Text("G             : GO");
            Push_Text("S             : SINGLE STEP");
            Push_Text("X             : SHOW REGISTERS");
            Push_Text("M       [ADDR]: SHOW MEMORY");
            Push_Text("D       [ADDR]: DISASSEMBLE");
            Push_Text("B <NUM> [ADDR]: SET BREAKPOINT");
            Push_Text("L             : LIST BREAKPOINTS");
            Push_Text("R <NUM>       : REMOVE BREAKPOINT");
            break;
        // Run
        case 'G':
            Z80_Step = FALSE;
            Z80_Pause = FALSE;
            break;
        // Single step
        case 'S':
            Z80_Step = TRUE;
            Z80_Pause = FALSE;
            break;
        // Show memory
        case 'M':
            i++;
            if(Cmd[i] != 0)
            {
                display_address = Get_Address(&Cmd[i], &pEnd);
                Push_Text("");
            }
            for(l = 0; l < 8; l++)
            {
                k = 0;
                // HEX
                for(j = 0; j < 16; j++)
                {
                    mem_value = z80.ReadMemory(display_address + j) & 0xff;
                    mem_line[k] = HEXTabASCII[(mem_value * 2)];
                    mem_line[k + 1] = HEXTabASCII[(mem_value * 2) + 1];
                    mem_line[k + 2] = ' ';
                    k += 3;
                }
                mem_line[k++] = ' ';
                mem_line[k++] = '\"';
                // ASCII
                for(j = 0; j < 16; j++)
                {
                    mem_value = z80.ReadMemory(display_address + j) & 0xff;
                    if(mem_value < 0x20) mem_value = '.';
                    if(mem_value > 0x7f) mem_value = '.';
                    mem_line[k] = mem_value;
                    k++;
                }
                mem_line[k++] = '\"';
                mem_line[k] = 0;
                Push_Text("$%.04x: %s", display_address, mem_line);
                display_address = display_address + 16;
                if(display_address > 0xffff)
                {
                    display_address -= 0x10000;
                }
            }
            break;
        // Print registers status + disassemble current PC
        case 'X':
            Print_Registers();
            break;
        // Disassemble
        case 'D':
            i++;
            if(Cmd[i] != 0)
            {
                display_address = Get_Address(&Cmd[i], &pEnd);
                Push_Text("");
            }
            for(l = 0; l < 12; l++)
            {
                display_address = Dis_Line(display_address);
            }
            break;
        // Set breakpoint
        case 'B':
            i++;
            bp_number = Get_Address(&Cmd[i], &pEnd);
            if(bp_number < 1 || bp_number > 8)
            {
                Push_Text("**WRONG BREAKPOINT NUMBER**");
                break;
            }
            if(!strlen(pEnd))
            {
                // Set the current PC if nothing is given
                display_address = z80.PC;
            }
            else
            {
                display_address = Get_Address(pEnd, &pEnd);
            }
            Breakpoints[bp_number - 1] = display_address;
            Push_Text("BP %d SET TO: $%.04X", bp_number, Breakpoints[bp_number - 1]);
            break;
        // list breakpoints
        case 'L':
            Push_Text("**BREAKPOINTS**");
            Push_Text("");
            for(j = 0; j < 8; j++)
            {
                if(Breakpoints[j] == -1)
                {
                    Push_Text("BP %d: %s", j + 1, "(Not Set)");
                }
                else
                {
                    Push_Text("BP %d: $%.04X", j + 1, Breakpoints[j]);
                }

            }
            break;
        case 'R':
            i++;
            bp_number = Get_Address(&Cmd[i], &pEnd);
            if(bp_number < 1 || bp_number > 8)
            {
                Push_Text("**WRONG BREAKPOINT NUMBER**");
                break;
            }
            Breakpoints[bp_number - 1] = -1;
            Push_Text("BP %d REMOVED", bp_number);
            break;
    }
}

void Draw(HWND hWnd, HDC hdc)
{
    int i;
    RECT rect;
	
    HDC hmemdc = CreateCompatibleDC(hdc);
    GetClientRect(hWnd, &rect);

    HBITMAP holdbmp = (HBITMAP) SelectObject(hmemdc, hbitmap);
	SetTextColor(hmemdc, 0xffffff);
	SetBkColor(hmemdc, 0);
	HFONT holdfont = (HFONT) SelectObject(hmemdc, hFont);
    for(i = 0; i < 31; i++)
    {
	    TextOut(hmemdc, 0, i * FontHeight, Buf[i], 80);
    }
	SelectObject(hmemdc, holdfont);
	BitBlt(hdc, 0, 0, rect.right, rect.bottom - 20, hmemdc, 0, 0, SRCCOPY);
	SelectObject(hmemdc, holdbmp);
	DeleteDC(hmemdc);
}

LRESULT CALLBACK Mon_Proc( HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    RECT WRect;
    int Top;
    int Left;
    int Width;
    int Height;
	PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
	{
		case WM_DESTROY:
            Monitor_Opened = FALSE;
            if(hFont != 0) DeleteObject(hFont);
	        if(hbitmap) DeleteObject(hbitmap);
            Z80_Step = FALSE;
            Z80_Pause = FALSE;
			break;
		case WM_ACTIVATE:
            SetFocus(hTextBox);
            return 0;
		case WM_ERASEBKGND:
            return 0;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			Draw(hWnd, hdc);
			EndPaint(hWnd, &ps);
            return 0;
		case WM_SIZE:
            GetClientRect(hWnd, &WRect);
            Top = WRect.top;
            Left = WRect.left;
            Height = WRect.bottom - Top;
            Width = WRect.right - Left;
            MoveWindow(hTextBox, Left, Height - 20, Width, 20, 1);
			break;
    }
    return DefWindowProc( hWnd, message, wparam, lparam );
}

LRESULT CALLBACK Text_Proc( HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    int i;
    switch( message )
	{
        case WM_COMMAND:
            if(HIWORD(wparam) == EN_CHANGE)
            {
                SendMessage(hTextBox, WM_GETTEXT, 255, (long) TempEntry);
                i = strlen(TempEntry);
                if(TempEntry[i - 1] == 10)
                {
                    // Remove the extra shit
                    TempEntry[i - 2] = 0;
                    TempEntry[i - 1] = 0;
                    Parse_Command(TempEntry);
                    SendMessage(hTextBox, WM_SETTEXT, 0, (long) "");
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }
            i = 0;
            break;
    }
    return(CallWindowProc((WNDPROC) GetWindowLong(hWnd, GWL_USERDATA), hWnd, message, wparam, lparam));
}

int Monitor_Is_Opened()
{
    return Monitor_Opened;
}

// -----------------------------------------------------------------------
// Create a font
HFONT CALLBACK Obtain_Font(char *FontNameToObtain, long FontSizeToObtain, HWND hWnd, long Bold, long Italic)
{
    HFONT ReturnValue = 0;
    LOGFONT WACMFont;
    int i = 0;
    HDC WAFhDC = 0;

    WAFhDC = GetDC(hWnd);
    WACMFont.lfHeight = -(long)((FontSizeToObtain * GetDeviceCaps(WAFhDC, LOGPIXELSY)) / 72);
    WACMFont.lfWidth = 0;
    WACMFont.lfEscapement = 0;
    WACMFont.lfOrientation = 0;
    if(Bold == 1) WACMFont.lfWeight = FW_BOLD;
    else WACMFont.lfWeight = FW_NORMAL;
    WACMFont.lfItalic = (BYTE) Italic;
    WACMFont.lfUnderline = 0;
    WACMFont.lfStrikeOut = 0;
    WACMFont.lfCharSet = DEFAULT_CHARSET;
    WACMFont.lfOutPrecision = OUT_OUTLINE_PRECIS;
    WACMFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    WACMFont.lfQuality = ANTIALIASED_QUALITY;
    WACMFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    strcpy(WACMFont.lfFaceName, FontNameToObtain);
    WACMFont.lfFaceName[i] = 0;
    ReturnValue = CreateFontIndirect(&WACMFont);
    ReleaseDC(hWnd, WAFhDC);
    return(ReturnValue);
}

void Monitor_Open()
{
    RECT rect;
    if(Monitor_Opened == FALSE)
    {
	    WNDCLASSEX wcex;
	    int window_width;
	    int window_height;
	    // Save the instance handle in a global variable
	    // Window class definition
	    wcex.cbSize = sizeof( WNDCLASSEX );
	    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	    wcex.lpfnWndProc	= (WNDPROC) Mon_Proc;
	    wcex.cbClsExtra		= 0;
	    wcex.cbWndExtra		= 0;
	    wcex.hInstance		= Instance;
	    wcex.hIcon			= LoadIcon( Instance, (LPCTSTR) IDI_ICON );
	    wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	    wcex.hbrBackground	= (HBRUSH)( COLOR_BACKGROUND);
	    wcex.lpszMenuName	= NULL;
	    wcex.lpszClassName	= "Monitor";
	    wcex.hIconSm		= LoadIcon( wcex.hInstance, (LPCTSTR)IDI_SMALL );
	    RegisterClassEx( &wcex );
	    // Adjust the window size
        window_width = 640;
        window_height = 480;
	    window_width += ( GetSystemMetrics( SM_CXSIZEFRAME ) * 2 );
	    window_height += ( GetSystemMetrics( SM_CYSIZEFRAME ) * 2 + GetSystemMetrics( SM_CYCAPTION ) );
	    // Window Creation
	    hWnd = CreateWindow( "Monitor",
						    "PHC-25 Monitor",
						    WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
						    CW_USEDEFAULT, CW_USEDEFAULT,
						    window_width, window_height,
						    NULL, NULL, Instance, NULL );
	    if( !hWnd )
	    {
		    return;
	    }

	    HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &rect);
	    hbitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        ReleaseDC(hWnd, hdc);

        hFont = Obtain_Font("Courier New", 10, 0, 0, 0);
	    FontHeight = 15;

        hTextBox = CreateWindowEx(WS_EX_STATICEDGE, "EDIT", "", 
                                  ES_MULTILINE | ES_LEFT | ES_UPPERCASE | ES_WANTRETURN | ES_AUTOVSCROLL | WS_VISIBLE | WS_CHILD,
                                  0, 20, 640, 20, hWnd, (HMENU) 0, Instance, NULL);
        if(hTextBox == 0) return;
        SetWindowLong(hWnd, GWL_USERDATA, SetWindowLong(hWnd, GWL_WNDPROC, (long) &Text_Proc));
        SendMessage(hTextBox, WM_SETFONT, (UINT) hFont, (long) 0);
        Z80_Pause = TRUE;
        Push_Text("**BREAK ('H' FOR HELP)**");
        Print_Registers();
        ShowWindow( hWnd, SW_SHOWNORMAL );
	    UpdateWindow( hWnd );
        SetFocus(hTextBox);
    }
    Monitor_Opened = TRUE;
}

void Monitor_Close()
{
    if(Monitor_Opened == TRUE)
    {
        DestroyWindow(hWnd);
    }
}

void Print_Registers_From_Outside(int show_breakpoint)
{
    if(Monitor_Is_Opened())
    {
        if(show_breakpoint)
        {
            Push_Text("**BREAKPOINT REACHED AT $%.04X**", z80.PC);
        }
        Print_Registers();
        InvalidateRect(hWnd, NULL, TRUE);
    }
}
