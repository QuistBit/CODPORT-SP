/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// win_syscon.h
#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#define COPY_ID         1
#define QUIT_ID         2
#define CLEAR_ID        3

#define ERRORBOX_ID     10
#define ERRORTEXT_ID    11

#define EDIT_ID         100
#define INPUT_ID        101

typedef struct
{
	HWND hWnd;
	HWND hwndBuffer;

	HWND hwndButtonClear;
	HWND hwndButtonCopy;
	HWND hwndButtonQuit;

	HWND hwndErrorBox;
	HWND hwndErrorText;

	HBRUSH hbrEditBackground;
	HBRUSH hbrErrorBackground;

	HFONT hfBufferFont;
	HFONT hfButtonFont;

	HWND hwndInputLine;

	char errorString[80];

	char consoleText[512], returnedText[512];
	int visLevel;
	qboolean quitOnClose;

	WNDPROC SysInputLineWndProc;

} WinConData;

static WinConData s_wcd;

/* The hidden native Edit control still lays out text synchronously. Keep
 * printable ASCII log text here until the console is shown. The original
 * byte-count rollover and ANSI conversion path remain authoritative; any
 * other byte flushes the pending text and uses that original path. This
 * affects only the hidden Win32 console, never the game console or logfile. */
#define CONBUF_APPEND_MESSAGE (WM_APP + 310)
static DWORD conbufThreadId;
static char conbufPending[32768];
static unsigned conbufPendingLength;
static int conbufPendingReplace;

static void Conbuf_FlushPending(void) {
 char text[32768];
 int replace;
 while(conbufPendingLength || conbufPendingReplace) {
  replace=conbufPendingReplace;
  memcpy(text,conbufPending,conbufPendingLength);
  text[conbufPendingLength]=0;
  conbufPendingLength=0;conbufPendingReplace=0;
  SendMessage(s_wcd.hwndBuffer,EM_SETSEL,replace?0:0xffff,replace?-1:0xffff);
  SendMessage(s_wcd.hwndBuffer,EM_LINESCROLL,0,0xffff);
  SendMessage(s_wcd.hwndBuffer,EM_SCROLLCARET,0,0);
  SendMessage(s_wcd.hwndBuffer,EM_REPLACESEL,0,(LPARAM)text);
 }
}

static int Conbuf_QueueAscii(const char *text,unsigned length,int replace) {
 unsigned i;
 if(s_wcd.visLevel || !s_wcd.hwndBuffer)return 0;
 for(i=0;i<length;i++) {
  unsigned char c=(unsigned char)text[i];
  if((c<32 && c!='\r' && c!='\n' && c!='\t') || c>126)return 0;
 }
 if(replace){conbufPendingLength=0;conbufPendingReplace=1;}
 if(length>=sizeof(conbufPending)-conbufPendingLength)return 0;
 memcpy(conbufPending+conbufPendingLength,text,length);
 conbufPendingLength+=length;
 return 1;
}


static LONG WINAPI ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	char *cmdString;
	static qboolean s_timePolarity;

	switch ( uMsg )
	{
	case CONBUF_APPEND_MESSAGE:
		Conbuf_AppendText((const char *)lParam);
		return 0;
	case WM_ACTIVATE:
		if ( LOWORD( wParam ) != WA_INACTIVE ) {
			SetFocus( s_wcd.hwndInputLine );
		}

		if ( com_viewlog && ( com_dedicated && !com_dedicated->integer ) ) {
			// if the viewlog is open, check to see if it's being minimized
			if ( com_viewlog->integer == 1 ) {
				if ( HIWORD( wParam ) ) {   // minimized flag
					Cvar_Set( "viewlog", "2" );
				}
			} else if ( com_viewlog->integer == 2 )   {
				if ( !HIWORD( wParam ) ) {      // minimized flag
					Cvar_Set( "viewlog", "1" );
				}
			}
		}
		break;

	case WM_CLOSE:
		if ( ( com_dedicated && com_dedicated->integer ) ) {
			cmdString = CopyString( "quit" );
			Sys_QueEvent( 0, SE_CONSOLE, 0, 0, strlen( cmdString ) + 1, cmdString );
		} else if ( s_wcd.quitOnClose )   {
			PostQuitMessage( 0 );
		} else
		{
			Sys_ShowConsole( 0, qfalse );
			Cvar_Set( "viewlog", "0" );
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		if ( ( HWND ) lParam == s_wcd.hwndBuffer ) {
			SetBkColor( ( HDC ) wParam, RGB( 86, 117, 100 ) );
			SetTextColor( ( HDC ) wParam, RGB( 0xff, 0xff, 0xff ) );
//			SetBkColor( ( HDC ) wParam, RGB( 0x00, 0x00, 0xB0 ) );
//			SetTextColor( ( HDC ) wParam, RGB( 0xff, 0xff, 0x00 ) );

			return ( long ) s_wcd.hbrEditBackground;
		} else if ( ( HWND ) lParam == s_wcd.hwndErrorBox )   {
			if ( s_timePolarity & 1 ) {
				SetBkColor( ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0xff, 0x0, 0x00 ) );
			} else
			{
				SetBkColor( ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0x00, 0x0, 0x00 ) );
			}
			return ( long ) s_wcd.hbrErrorBackground;
		}
		break;

	case WM_COMMAND:
		if ( wParam == COPY_ID ) {
			Conbuf_FlushPending();
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
		} else if ( wParam == QUIT_ID )   {
			if ( s_wcd.quitOnClose ) {
				PostQuitMessage( 0 );
			} else
			{
				cmdString = CopyString( "quit" );
				Sys_QueEvent( 0, SE_CONSOLE, 0, 0, strlen( cmdString ) + 1, cmdString );
			}
		} else if ( wParam == CLEAR_ID )   {
			conbufPendingLength=0;conbufPendingReplace=0;
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) "" );
			UpdateWindow( s_wcd.hwndBuffer );
		}
		break;
	case WM_CREATE:
		s_wcd.hbrEditBackground = CreateSolidBrush( RGB( 86, 117, 100 ) );
//		s_wcd.hbrEditBackground = CreateSolidBrush( RGB( 0x00, 0x00, 0xB0 ) );
		s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x80, 0x80, 0x80 ) );
		SetTimer( hWnd, 1, 1000, NULL );
		break;
	case WM_TIMER:
		if ( wParam == 1 ) {
			s_timePolarity = !s_timePolarity;
			if ( s_wcd.hwndErrorBox ) {
				InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
			}
		}
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

LONG WINAPI InputLineWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	char inputBuffer[1024];

	switch ( uMsg )
	{
	case WM_KILLFOCUS:
		if ( ( HWND ) wParam == s_wcd.hWnd ||
			 ( HWND ) wParam == s_wcd.hwndErrorBox ) {
			SetFocus( hWnd );
			return 0;
		}
		break;

	case WM_CHAR:
		if ( wParam == 13 ) {
			GetWindowText( s_wcd.hwndInputLine, inputBuffer, sizeof( inputBuffer ) );
			strncat( s_wcd.consoleText, inputBuffer, sizeof( s_wcd.consoleText ) - strlen( s_wcd.consoleText ) - 5 );
			strcat( s_wcd.consoleText, "\n" );
			SetWindowText( s_wcd.hwndInputLine, "" );

			Sys_Print( va( "]%s\n", inputBuffer ) );

			return 0;
		}
	}

	return CallWindowProc( s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam );
}

/*
** Sys_CreateConsole
*/
void Sys_CreateConsole( void ) {
	HDC hDC;
	WNDCLASS wc;
	RECT rect;
	const char *DEDCLASS = "Wolf WinConsole";
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;

	conbufThreadId=GetCurrentThreadId();
	conbufPendingLength=0;conbufPendingReplace=0;
	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_wv.hInstance;
	wc.hIcon         = LoadIcon( g_wv.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL,IDC_ARROW );
	wc.hbrBackground = (void *)COLOR_WINDOW;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClass( &wc ) ) {
		return;
	}

	rect.left = 0;
	rect.right = 540;
	rect.top = 0;
	rect.bottom = 450;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	hDC = GetDC( GetDesktopWindow() );
	swidth = GetDeviceCaps( hDC, HORZRES );
	sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );


	s_wcd.hWnd = CreateWindowEx( 0,
								 DEDCLASS,
								 COD1_WINDOW_TITLE " Console",
								 DEDSTYLE,
								 ( swidth - 600 ) / 2, ( sheight - 450 ) / 2, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
								 NULL,
								 NULL,
								 g_wv.hInstance,
								 NULL );

	if ( s_wcd.hWnd == NULL ) {
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	nHeight = -MulDiv( 8, GetDeviceCaps( hDC, LOGPIXELSY ), 72 );

	s_wcd.hfBufferFont = CreateFont( nHeight,
									 0,
									 0,
									 0,
									 FW_LIGHT,
									 0,
									 0,
									 0,
									 DEFAULT_CHARSET,
									 OUT_DEFAULT_PRECIS,
									 CLIP_DEFAULT_PRECIS,
									 DEFAULT_QUALITY,
									 FF_MODERN | FIXED_PITCH,
									 "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
										ES_LEFT | ES_AUTOHSCROLL,
										6, 400, 528, 20,
										s_wcd.hWnd,
										( HMENU ) INPUT_ID,         // child window ID
										g_wv.hInstance, NULL );

	//
	// create the buttons
	//
	s_wcd.hwndButtonCopy = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
										 5, 425, 72, 24,
										 s_wcd.hWnd,
										 ( HMENU ) COPY_ID,         // child window ID
										 g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "copy" );

	s_wcd.hwndButtonClear = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
										  82, 425, 72, 24,
										  s_wcd.hWnd,
										  ( HMENU ) CLEAR_ID,       // child window ID
										  g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM ) "clear" );

	s_wcd.hwndButtonQuit = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
										 462, 425, 72, 24,
										 s_wcd.hWnd,
										 ( HMENU ) QUIT_ID,         // child window ID
										 g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "quit" );


	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
									 ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
									 6, 40, 526, 354,
									 s_wcd.hWnd,
									 ( HMENU ) EDIT_ID,             // child window ID
									 g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLong( s_wcd.hwndInputLine, GWL_WNDPROC, ( long ) InputLineWndProc );
	SendMessage( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT );
	UpdateWindow( s_wcd.hWnd );
	SetForegroundWindow( s_wcd.hWnd );
	SetFocus( s_wcd.hwndInputLine );

	s_wcd.visLevel = 1;
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole( void ) {
	if ( s_wcd.hWnd ) {
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
}

/*
** Sys_ShowConsole
*/
void Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
	s_wcd.quitOnClose = quitOnClose;

	if ( visLevel == s_wcd.visLevel ) {
		return;
	}

	if(visLevel && s_wcd.hWnd)Conbuf_FlushPending();
	s_wcd.visLevel = visLevel;

	if ( !s_wcd.hWnd ) {
		return;
	}

	switch ( visLevel )
	{
	case 0:
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		break;
	case 1:
		ShowWindow( s_wcd.hWnd, SW_SHOWNORMAL );
		SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		break;
	case 2:
		ShowWindow( s_wcd.hWnd, SW_MINIMIZE );
		break;
	default:
		Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}

/*
** Sys_ConsoleInput
*/
char *Sys_ConsoleInput( void ) {
	if ( s_wcd.consoleText[0] == 0 ) {
		return NULL;
	}

	strcpy( s_wcd.returnedText, s_wcd.consoleText );
	s_wcd.consoleText[0] = 0;

	return s_wcd.returnedText;
}

/*
** Conbuf_AppendText
*/
void Conbuf_AppendText( const char *pMsg ) {
#define CONSOLE_BUFFER_SIZE     16384

	char buffer[CONSOLE_BUFFER_SIZE * 2];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;
	int replace;
	if(conbufThreadId && GetCurrentThreadId()!=conbufThreadId && s_wcd.hWnd){
		SendMessage(s_wcd.hWnd,CONBUF_APPEND_MESSAGE,0,(LPARAM)pMsg);return;
	}

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 ) {
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	} else
	{
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) )
	{
		if ( msg[i] == '\n' && msg[i + 1] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		} else if ( msg[i] == '\r' )     {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( msg[i] == '\n' )     {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( Q_IsColorString( &msg[i] ) )   {
			i++;
		} else
		{
			*b = msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;


 s_totalChars += bufLen;
 replace=s_totalChars>0x7fff;
 if(replace)s_totalChars=bufLen;
 if(Conbuf_QueueAscii(buffer,(unsigned)bufLen,replace))return;
 if(replace){conbufPendingLength=0;conbufPendingReplace=0;}
 else Conbuf_FlushPending();
 SendMessage(s_wcd.hwndBuffer,EM_SETSEL,replace?0:0xffff,replace?-1:0xffff);
 SendMessage(s_wcd.hwndBuffer,EM_LINESCROLL,0,0xffff);
 SendMessage(s_wcd.hwndBuffer,EM_SCROLLCARET,0,0);
 SendMessage(s_wcd.hwndBuffer,EM_REPLACESEL,0,(LPARAM)buffer);
}

/*
** Sys_SetErrorText
*/
void Sys_SetErrorText( const char *buf ) {
	Q_strncpyz( s_wcd.errorString, buf, sizeof( s_wcd.errorString ) );

	if ( !s_wcd.hwndErrorBox ) {
		s_wcd.hwndErrorBox = CreateWindow( "static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
										   6, 5, 526, 30,
										   s_wcd.hWnd,
										   ( HMENU ) ERRORBOX_ID,           // child window ID
										   g_wv.hInstance, NULL );
		SendMessage( s_wcd.hwndErrorBox, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
		SetWindowText( s_wcd.hwndErrorBox, s_wcd.errorString );

		DestroyWindow( s_wcd.hwndInputLine );
		s_wcd.hwndInputLine = NULL;
	}
}
