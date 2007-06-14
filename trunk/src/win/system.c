/* ************************************************************************ */
/*																			*/
/*  ScreenWeaver HX															*/
/*  Copyright (c)2006 Edwin van Rijkom, Nicolas Cannasse					*/
/*																			*/
/* This library is free software; you can redistribute it and/or			*/
/* modify it under the terms of the GNU Lesser General Public				*/
/* License as published by the Free Software Foundation; either				*/
/* version 2.1 of the License, or (at your option) any later version.		*/
/*																			*/
/* This library is distributed in the hope that it will be useful,			*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of			*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU		*/
/* Lesser General Public License or the LICENSE file for more details.		*/
/*																			*/
/* ************************************************************************ */
#include <windows.h>
#include <stddef.h>

#define WM_MOUSEWHEEL 0x020A
#define FLASH_PLACEHOLDER_ID "SWFlash_PlaceholderX"

#include "../system.h"
#include "../flash_dll.h"
#include "../flash.h"

window_list *windows = NULL;
extern flash_dll *fl_dll;

// -------------------------------------- 64-bit compat ----------------------------------
// GetWindowLongPtr and SetWindowLongPtr should not issue a 64-bit compatibility
// complain when used with a pointer, but looks like VC headers are not correct
// these wrappers ensure then that further code is 64-bit warning-free

#pragma warning( disable : 4047 )

void *getptr( HWND h, LONG w ) {
	return GetWindowLongPtr(h,w);
}

void setptr( HWND h, LONG w, void *p ) {
	SetWindowLongPtr(h,w,p);
}

#pragma warning( default : 4047 )

// -------------------------------------- LIBRARY -----------------------------------------

library *system_library_open( const char *path ) {
	return (library*)LoadLibrary(path);
}

void *system_library_symbol( library *l, const char *symbol ) {
	return GetProcAddress((HMODULE)l,symbol);
}

void system_library_close( library *l ) {
	FreeLibrary((HMODULE)l);
}

// -------------------------------------- SYSTEM -----------------------------------------

static LRESULT CALLBACK WndProcStub( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
static void updateFlashMetrics(window *, int width, int height);
static void applyFlashMetrics(window *w);
static void fullscreen(window *w, bool enter);
static LRESULT sendEvent( window *w, UINT msg, WPARAM wParam, LPARAM lParam );
static LARGE_INTEGER hdtimer_frequency = {0,0};

#define CLASS_NAME "SWHXWindow"

#define AC_SRC_ALPHA 0x01
#define WS_EX_LAYERED 0x00080000
#define ULW_ALPHA 0x00000002
#define WM_SYNCCALL		(WM_USER + 0xFD)

typedef struct _queue {
	gen_callback f;
	void *param;
	struct _queue *next;
} mqueue;

typedef BOOL WINAPI UpdateLayeredWindowProc(HWND,HDC,POINT *,SIZE *,HDC,POINT *,COLORREF,BLENDFUNCTION *,DWORD);
static UpdateLayeredWindowProc* pUpdateLayeredWindow = NULL;
static HMODULE user32 = NULL;
static DWORD main_thread_id;
static mqueue *main_queue = NULL, *main_head = NULL;
static CRITICAL_SECTION main_lock;

int system_init() {
	WNDCLASSEX wcl;
	HINSTANCE hinst = GetModuleHandle(NULL);
	memset(&wcl,0,sizeof(wcl));
	wcl.cbSize			= sizeof(WNDCLASSEX);
	wcl.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcl.lpfnWndProc		= WndProcStub;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hInstance		= hinst;
	wcl.hIcon			= LoadIcon(hinst,MAKEINTRESOURCE(104));
	wcl.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wcl.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcl.lpszMenuName	= "";
	wcl.lpszClassName	= CLASS_NAME;
	wcl.hIconSm			= 0;
	if( RegisterClassEx(&wcl) == 0 )
		return -1;
	main_thread_id = GetCurrentThreadId();
	// this will create our main message queue
	PeekMessage(NULL,NULL,0,0,0);
	QueryPerformanceFrequency(&hdtimer_frequency);
	user32 = LoadLibrary("user32.dll");
	if (user32) {
		pUpdateLayeredWindow = (UpdateLayeredWindowProc*) GetProcAddress(user32,"UpdateLayeredWindow");
		if (!pUpdateLayeredWindow)
			FreeLibrary(user32);
	}
	InitializeCriticalSection(&main_lock);

	return 0;
}

void system_cleanup() {
	// close user32 (used for UpdateLayeredWindow)
	if (user32) {
		FreeLibrary(user32);
		pUpdateLayeredWindow = NULL;
		user32 = NULL;
	}
}

void system_sync_call( gen_callback func, void *param ) {
	// adds message to the queue
	mqueue *m = malloc(sizeof(mqueue));
	m->f = func;
	m->param = param;
	m->next = NULL;
	EnterCriticalSection(&main_lock);
	if( main_queue == NULL )
		main_head = m;
	else
		main_queue->next = m;
	main_queue = m;
	LeaveCriticalSection(&main_lock);
	// notice the main thread
	PostThreadMessage(main_thread_id,WM_SYNCCALL,0,0);
}

int system_is_main_thread() {
	DWORD current = GetCurrentThreadId();
	return current == main_thread_id;
}

char *system_fullpath( const char *file ) {
	char path[MAX_PATH];
	char prefixed[MAX_PATH];
	if( GetFullPathName(file,MAX_PATH - 8,path,NULL) == 0 )
		return strdup(file);
	strcpy(prefixed,"file:///");
	strcat(prefixed,path);
	return strdup(prefixed);
}

char *system_plugin_file_version(const char *p) {
	char *result = NULL;
	DWORD handle;
	DWORD size = GetFileVersionInfoSize(p,&handle);

	if (size) {
		void *data = malloc(size);
		if (GetFileVersionInfo(p,handle,size,data)) {
			char* version;
			if (VerQueryValue(data,"\\StringFileInfo\\040904E4\\ProductVersion",&version,&size)) {
				result = strdup(version);
			}
		}
		free(data);
	}
	return result;
}

// -------------------------------------- WINDOW -----------------------------------------

typedef void SetBufferProc(void*);
typedef void PaintBufferProc(void*,void*);

struct _window {
	HWND hwnd;
	HDC hdc;
	NPWindow npwin;
	void *flash;
	on_event evt;
	on_npevent npevent;
	private_data *p;
	enum WindowFlags flags;
	// full screen data:
	WINDOWINFO fullscreen_org_wi;
	// place holder data:
	HWND flash_ph_hwnd;
	WNDPROC flash_ph_wndproc;
	LARGE_INTEGER flash_ph_lastum;
	// back buffer:
	SetBufferProc *bbuffer_set;
	PaintBufferProc *bbuffer_paint;
	HBITMAP bbuffer_bmp;
	HDC bbuffer_hdc;
	DWORD* bbuffer_bits;
	// back buffer B, for alpha calcs.
	HBITMAP bbufferB_bmp;
	HDC bbufferB_hdc;
	DWORD* bbufferB_bits;
	// window placement
	WINDOWPLACEMENT *placement_org;
	int	placement_int;
	// message hook
	msg_hook_list *msg_hooks;
};

static void setBackBufferStd(window *w);
static void setBackBufferTrans(window *w);
static void paintBackBufferStd(window *w, NPRect *rc);
static void paintBackBufferTrans(window *w, NPRect *rc);

window *system_window_create( const char *title, int width, int height, enum WindowFlags flags, on_event f ) {
	RECT rc = { 0, 0, width, height };
	HWND hwnd;
	NPRect npclip;
	int style;
	DWORD exstyle = 0;
	window *w = malloc(sizeof(struct _window));
	memset(w,0,sizeof(struct _window));
	w->flags = flags ^ WF_FLASH_RUNNING;
	w->npwin.width = width;
	w->npwin.height = height;

	if ( flags & WF_ALWAYS_ONTOP )
		exstyle |= WS_EX_TOPMOST;
	if ( flags & WF_NO_TASKBAR )
		exstyle |= WS_EX_TOOLWINDOW;

	if (flags & WF_TRANSPARENT && pUpdateLayeredWindow) {
		exstyle |=  0x80000; //== WS_EX_LAYERED;
		style = WS_SYSMENU | WS_POPUP;
		w->bbuffer_set = setBackBufferTrans;
		w->bbuffer_paint = paintBackBufferTrans;
	} else {
		if (flags & WF_PLAIN) {
			style = WS_POPUP | WS_BORDER | WS_SYSMENU;
			exstyle |= WS_EX_APPWINDOW;
		} else {
			style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		}
		AdjustWindowRectEx( &rc, style, FALSE, exstyle);
		w->bbuffer_set = setBackBufferStd;
		w->bbuffer_paint = paintBackBufferStd;
	}
	hwnd = CreateWindowEx(exstyle, CLASS_NAME,title,style,CW_USEDEFAULT,0,rc.right-rc.left,rc.bottom-rc.top,NULL,NULL,GetModuleHandle(NULL),0);
	if( hwnd == NULL ) {
		free((void*)w);
		return NULL;
	}

	w->hwnd = hwnd;
	w->hdc = GetWindowDC(hwnd);
	w->evt = f;

	npclip.top = 0;
	npclip.left = 0;

	w->npwin.type = NPWindowTypeDrawable;
	w->npwin.clipRect = npclip;
	w->npwin.x = 0;
	w->npwin.y = 0;

	setptr(hwnd,GWLP_USERDATA,w);
	updateFlashMetrics(w,width,height);

	return w;
}

static void updateFlashMetrics(window *w, int width, int height) {
	RECT rc = {0,0,w->npwin.width,w->npwin.height};
	w->npwin.clipRect.bottom = height;
	w->npwin.clipRect.right = width;
	w->npwin.height = height;
	w->npwin.width = width;
	w->bbuffer_set(w);
}

static void applyFlashMetrics(window *w) {
	if( w->flash ) {
		fl_dll->table.setwindow(flashp_get_npp(w->flash),&w->npwin);
		system_window_invalidate(w,&w->npwin.clipRect);
	}
}

static void setBackBufferStd(window *w) {
	HDC hdc_desk = GetDC(0);
	if (w->bbuffer_hdc) {
		SelectObject(w->bbuffer_hdc, NULL);
		DeleteObject(w->bbuffer_bmp);
	} else {
		w->bbuffer_hdc = CreateCompatibleDC(NULL);
	}
	w->bbuffer_bmp = CreateCompatibleBitmap(hdc_desk,w->npwin.width,w->npwin.height);
	ReleaseDC(0,hdc_desk);
	SelectObject(w->bbuffer_hdc,w->bbuffer_bmp);
	w->npwin.window = w->bbuffer_hdc;
}

static HBITMAP createDIB( long width, long height, LPVOID pixels, BYTE bitsperpixel, DWORD * mask) {
	HBITMAP result = 0;
	BITMAPV4HEADER bih;
	memset(&bih, 0, sizeof(bih) );
	bih.bV4Size				= sizeof(BITMAPV4HEADER);
	bih.bV4Width			= width;
	bih.bV4Height			= -height;
	bih.bV4Planes			= 1;
	bih.bV4BitCount			= bitsperpixel ? bitsperpixel : GetDeviceCaps(GetDC(0), BITSPIXEL);
	bih.bV4V4Compression	= mask? BI_BITFIELDS : BI_RGB;

	if (mask){
		bih.bV4RedMask		= mask[0];
		bih.bV4GreenMask	= mask[1];
		bih.bV4BlueMask		= mask[2];
		bih.bV4AlphaMask	= mask[3];
	}

	return CreateDIBSection(GetDC(0),(LPBITMAPINFO) &bih, DIB_RGB_COLORS, (LPVOID *)pixels, NULL, 0);
}

static void setBackBufferTrans(window *w) {
	static DWORD bmMask[4] = {0xFF0000,0xFF00,0xFF,0xFF000000};

	if (w->bbuffer_bmp) {
		SelectObject(w->bbuffer_hdc, NULL);
		DeleteObject(w->bbuffer_bmp);
		SelectObject(w->bbufferB_hdc, NULL);
		DeleteObject(w->bbufferB_bmp);
	}

	w->bbuffer_bmp = createDIB(w->npwin.width,w->npwin.height,&w->bbuffer_bits,32,(DWORD*)&bmMask);
	w->bbufferB_bmp = createDIB(w->npwin.width,w->npwin.height,&w->bbufferB_bits,32,(DWORD*)&bmMask);

	if (!w->bbuffer_hdc) {
		w->bbuffer_hdc = CreateCompatibleDC(NULL);
		w->bbufferB_hdc = CreateCompatibleDC(NULL);
	}

	SelectObject(w->bbuffer_hdc,w->bbuffer_bmp);
	SelectObject(w->bbufferB_hdc,w->bbufferB_bmp);

	w->npwin.window = w->bbuffer_hdc;
}

static void deleteBackBuffer(window *w) {
	SelectObject(w->bbuffer_hdc, NULL);
	DeleteObject(w->bbuffer_bmp);
	w->bbuffer_bmp = NULL;
	DeleteDC(w->bbuffer_hdc);
	w->bbuffer_hdc = NULL;
	if (w->bbufferB_hdc) {
		SelectObject(w->bbufferB_hdc, NULL);
		DeleteObject(w->bbufferB_bmp);
		w->bbufferB_bmp = NULL;
		DeleteDC(w->bbufferB_hdc);
		w->bbufferB_hdc = NULL;
	}
}

void system_window_show( window *w, int show ) {
	ShowWindow( w->hwnd, show?SW_SHOW:SW_HIDE );
}

void system_window_destroy( window *w ) {
	if( w->hwnd )
		DestroyWindow(w->hwnd);
}

void system_window_set_npevent( window *w, on_npevent f ) {
	w->npevent = f;
}

void system_window_set_private( window *w, private_data *p ) {
	w->p = p;
}

NPWindow *system_window_getnp( window *w ) {
	return &w->npwin;
}

private_data *system_window_get_private( window *w ) {
	return w->p;
}

void *system_window_get_handle( window *w ) {
	return w->hwnd;
}

msg_hook_list **system_window_get_msg_hook_list( window *w ) {
	return &w->msg_hooks;
}

static void onFlashStart(window *w);
static void onFlashStop(window *w);

#define SET(v,flags,toggle) v = toggle ? (v | flags) : (v & ~flags)
#define IS_SET(v,flags)	(((v) & flags) == flags)

void system_window_set_prop( window *w, enum WindowProperty prop, int value ) {
	DWORD style = GetWindowLong(w->hwnd,GWL_STYLE);
	switch( prop ) {
		case WP_RESIZABLE:
			if (!(w->flags & WF_PLAIN))
				SET(style,WS_THICKFRAME,value);
			break;
		case WP_MAXIMIZE_ICON:
			SET(style,WS_MAXIMIZEBOX ,value);
			break;
		case WP_MINIMIZE_ICON:
			SET(style,WS_MINIMIZEBOX ,value);
			break;
		case WP_WIDTH:
		case WP_HEIGHT:	{
			RECT rc;
			GetClientRect(w->hwnd,&rc);
			if( prop == WP_WIDTH )
				rc.right = value;
			else
				rc.bottom = value;
			AdjustWindowRectEx( &rc, style, FALSE, 0);
			SetWindowPos(w->hwnd,NULL,0,0,rc.right - rc.left,rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
			return;
		}
		case WP_TOP:
		case WP_LEFT: {
			RECT rc;
			GetWindowRect(w->hwnd,&rc);
			if( prop == WP_LEFT )
				rc.left = value;
			else
				rc.top = value;
			SetWindowPos(w->hwnd,NULL,rc.left,rc.top,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
			return;
		}
		case WP_FULLSCREEN:
			if( w->flags & WF_FULLSCREEN ) {
				if( !value ) {
					fullscreen(w, 0);
					w->flags ^= WF_FULLSCREEN;
				}
			} else if( value ) {
				fullscreen(w, 1);
				w->flags |= WF_FULLSCREEN;
			}
			return;
		case WP_FLASH_RUNNING:
			value? onFlashStart(w) : onFlashStop(w);
			return;
		case WP_DROPTARGET:
			DragAcceptFiles(w->hwnd,value!=0);
			if (value)
				w->flags &= ~WF_DROPTARGET;
			else
				w->flags |= WF_DROPTARGET;
			return;
		case WP_MINIMIZED:
			if (value && !IsIconic(w->hwnd)) {
				w->placement_int = 1;
				SendMessage(w->hwnd,WM_SYSCOMMAND,SC_MINIMIZE,0);
			} else if (!value && IsIconic(w->hwnd)) {
				w->placement_int = 1;
				SendMessage(w->hwnd,WM_SYSCOMMAND,SC_RESTORE,0);
			}
			break;
		case WP_MAXIMIZED: {
			if (!w->placement_org && value) {
				int s = sizeof(WINDOWPLACEMENT);
				w->placement_org = malloc(s);
				w->placement_org->length = s;
				GetWindowPlacement(w->hwnd,w->placement_org);
				w->placement_int = 1;
				SendMessage(w->hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);
			} else if(w->placement_org && !value) {
				w->placement_int = 1;
				SetWindowPlacement(w->hwnd,w->placement_org);
				free(w->placement_org);
				w->placement_org = 0;
			}
			break;
		}

	}
	SetWindowLong(w->hwnd,GWL_STYLE,style);
}

int system_window_get_prop( window *w, enum WindowProperty prop ) {
	DWORD style = GetWindowLong(w->hwnd,GWL_STYLE);
	RECT rc;
	switch( prop ) {
		case WP_RESIZABLE:
			return IS_SET(style,WS_THICKFRAME);
		case WP_MAXIMIZE_ICON:
			return IS_SET(style,WS_MAXIMIZEBOX);
		case WP_MINIMIZE_ICON:
			return IS_SET(style,WS_MINIMIZEBOX);
		case WP_WIDTH:
			GetClientRect(w->hwnd,&rc);
			return rc.right;
		case WP_HEIGHT:
			GetClientRect(w->hwnd,&rc);
			return rc.bottom;
		case WP_TOP:
			GetWindowRect(w->hwnd,&rc);
			return rc.top;
		case WP_LEFT:
			GetWindowRect(w->hwnd,&rc);
			return rc.left;
		case WP_FULLSCREEN:
			return (w->flags & WF_FULLSCREEN);
		case WP_TRANSPARENT:
			return (w->flags & WF_TRANSPARENT);
		case WP_DROPTARGET:
			return (w->flags & WF_DROPTARGET);
		case WP_MINIMIZED:
			return IsIconic(w->hwnd);
		case WP_MAXIMIZED:
			return w->placement_org? 1 : 0;
	}
	return 0;
}

void system_window_set_flash( window *w, void *f ) {
	w->flash = f;
}

void system_window_set_title( window *w, const char* title ) {
	SetWindowText(w->hwnd, title);
}

void system_window_drag( window *w ) {
	SendMessage(w->hwnd,WM_SYSCOMMAND,SC_MOVE+1,0);
}

void system_window_resize( window *w, int o ) {
	SendMessage(w->hwnd,WM_SYSCOMMAND,SC_SIZE+o,0);
}

BOOL CALLBACK findFlashPlaceHolderWindow( HWND hwnd, LPARAM lParam ) {
	char cClassname[255];
	*((HWND*) lParam) = 0;
	GetClassName(hwnd,(LPTSTR)&cClassname,255);
	if (stricmp(FLASH_PLACEHOLDER_ID,cClassname)==0) {
		window_list * win = windows;
		while(win) {
			if (win->w->flash_ph_hwnd == hwnd)
				return 1;
			win = win->next;
		}
		*((HWND*) lParam) = hwnd;
		return 0;
	}
	return 1;
}

static LRESULT CALLBACK WndProcPH( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// obtain pointer to window instance concerned:
	window_list *win = windows;
	window *w = NULL;
	while(win) {
		if (win->w->flash_ph_hwnd == hwnd) {
			w = win->w;
			break;
		}
		win = win->next;
	}
	if (!hwnd || !w) return 0;

	// do magic:
	switch (msg) {
		case WM_USER+1:
			QueryPerformanceCounter(&w->flash_ph_lastum);
			break;
		case WM_TIMER:
			if (wparam == 3) {
				LARGE_INTEGER counter;
				QueryPerformanceCounter(&counter);
				// See if Flash's last frame-pulse was longer than a second ago:

				/*
					2007/04/26 : disable that for now, cause "unresponsive UI" bugs as described by Marc Hughes

				if (counter.QuadPart - w->flash_ph_lastum.QuadPart > hdtimer_frequency.QuadPart)
					// Sending a timer event with ID 1 seems to revive Flash's frame
					// pulse. May have side-effects - keep an eye open for it
					sendEvent(w, msg, 1, 0);


				*/
			}
			break;
		default:
#ifdef _DEBUG
		{
			char out[255];
			sprintf(out,"PH msg: % i [0x%X] %i %i\n", msg, msg, wparam, lparam);
			//OutputDebugString(out);
		}
#endif
			break;
	}

	// Forward message to original handler:
	return CallWindowProc(w->flash_ph_wndproc,hwnd,msg,wparam,lparam);
}

void onFlashStart( window *w ) {
	HWND hwnd = NULL;
	EnumThreadWindows(GetCurrentThreadId(),findFlashPlaceHolderWindow, (LPARAM) &hwnd );
	if (hwnd) {
		w->flash_ph_hwnd = hwnd;
		w->flash_ph_wndproc = getptr(hwnd,GWL_WNDPROC);
		// subclass the Flash place holder window:
		setptr(hwnd,GWL_WNDPROC,WndProcPH);
	}
}

void onFlashStop( window *w ) {
	if (w->flash_ph_hwnd) {
		// un-subclass the place holder window:
		setptr(w->flash_ph_hwnd,GWL_WNDPROC,w->flash_ph_wndproc);
		w->flash_ph_wndproc = NULL;
		w->flash_ph_hwnd = NULL;
	}
}

void system_loop() {
	MSG msg;
	while( GetMessage(&msg,NULL,0,0) ) {
		if( msg.message != WM_SYNCCALL && TranslateAccelerator(0, 0, &msg) == 0 ) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
		}
		while( main_head ) {
			mqueue m;
			EnterCriticalSection(&main_lock);
			if( main_head == NULL ) {
				LeaveCriticalSection(&main_lock);
				break;
			}
			m = *main_head;
			free(main_head);
			main_head = m.next;
			if( main_head == NULL )
				main_queue = NULL;
			LeaveCriticalSection(&main_lock);
			m.f(m.param);
		}
	}
}

void system_loop_exit() {
	PostQuitMessage(0);
}

static LRESULT sendEvent( window *w, UINT msg, WPARAM wParam, LPARAM lParam ) {
	NPEvent event;
	if( w->npevent == NULL )
		return 0;
	memset(&event, 0, sizeof( NPEvent ));
	event.event  = msg;
	event.lParam = lParam;
	event.wParam = wParam;
	return w->npevent(w,&event);
}

void system_window_invalidate( window *w, NPRect *r ) {
	r->right = min(w->npwin.width,r->right);
	r->bottom = min(w->npwin.height,r->bottom);
	// might crash with transparency ??
	fl_dll->table.setwindow(flashp_get_npp(w->flash),&w->npwin);
	w->bbuffer_paint(w,r);
}

void paintBackBufferStd( window *w, NPRect *r) {
	RECT rc = {r->left,r->top,r->right,r->bottom};
	HRGN rgn = CreateRectRgn(r->left,r->top,r->right,r->bottom);
	// restrict drawing to rectangle that needs updating:
	SelectClipRgn(w->bbuffer_hdc,rgn);
	DeleteObject(rgn);
	// have Flash draw:
	sendEvent(w, WM_PAINT, 0, 0);
#ifdef _DEBUG
	{	// Draw a green rectangle indicating the updated
		// area.
		HGDIOBJ sel;
		HPEN green = CreatePen(PS_SOLID,1,RGB(0,0xff,0));
		sel = SelectObject(w->bbuffer_hdc,green);
		MoveToEx(w->bbuffer_hdc, r->left, r->top, (LPPOINT) NULL);
		LineTo(w->bbuffer_hdc, r->left, r->bottom-1);
		LineTo(w->bbuffer_hdc, r->right-1, r->bottom-1);
		LineTo(w->bbuffer_hdc, r->right-1,r->top);
		LineTo(w->bbuffer_hdc, r->left,r->top);
		SelectObject(w->bbuffer_hdc,sel);
		DeleteObject(green);
	}
#endif
	// have our window reflect the updated Flash graphics:
	InvalidateRect(w->hwnd,&rc,FALSE);
	SendMessage(w->hwnd,WM_PAINT,0,0);
}

void paintBackBufferTrans( window *w, NPRect *r) {
	static BLENDFUNCTION bf = {0,0,0xFF,AC_SRC_ALPHA};
	static POINT pos = {0,0};
	SIZE size = {w->npwin.width,w->npwin.height};
	long i,x,xm,y,ym,offset,offsetl;
	BYTE a;
	BYTE *ba = (BYTE*) w->bbuffer_bits;
	BYTE *bb = (BYTE*) w->bbufferB_bits;
	HRGN rgn = CreateRectRgn(r->left,r->top,r->right,r->bottom);

	// restrict drawing to rectangle that needs updating:
	SelectClipRgn(w->bbuffer_hdc,rgn);
	SelectClipRgn(w->bbufferB_hdc,rgn);
	DeleteObject(rgn);

	GdiFlush();

	// Flash can't deliver us a pre-multiplied alpha channel
	// bearing image of itself directly. This is however what we
	// need to feed UpdateLayeredWindow.
	// We can only request Flash to draw transparently over
	// an existing image. In order to create a pre-mult. alhpa
	// bearing image, we have it drawn twice: once on white and
	// once on black bg. Any transparent pixels will have changed
	// value between the two draws. By substracting them, and
	// taking the inverse, we obtain the alpha channel. Last,
	// we pre-multiply the rgb values with the found alpha
	// value.

	// pre-calculate freq. used values:
	offset = w->npwin.clipRect.right*r->top + r->left;
	ym = r->bottom - r->top;
	xm = r->right - r->left;

	// Prepare black and white backgrounds:
	for (y=0; y<ym; y++) {
		offsetl = offset + y*w->npwin.clipRect.right;
		for (x=0; x<xm; x++) {
			w->bbuffer_bits[offsetl + x] = 0xFFFFFFFF;
			w->bbufferB_bits[offsetl + x] = 0xFF000000;
		}
	}

	w->npwin.window = w->bbufferB_hdc;
	sendEvent(w,WM_PAINT,0,0);

	w->npwin.window = w->bbuffer_hdc;
	sendEvent(w,WM_PAINT,0,0);

	for (y=0; y<ym; y++) {
		offsetl = (offset + y*w->npwin.clipRect.right) * 4;
		for (x=0; x<xm; x++) {
			i = offsetl + x*4;
			a = ~(ba[i]-bb[i]);
			ba[i] = (ba[i]*a) >> 8;
			ba[i+1] = (ba[i+1]*a) >> 8;
			ba[i+2] = (ba[i+2]*a) >> 8;
			ba[i+3] = a;
		}
	}
	pUpdateLayeredWindow(w->hwnd,w->bbuffer_hdc,0,&size,w->bbuffer_hdc,&pos,0,&bf,ULW_ALPHA);

/*
#ifdef _DEBUG
	{	// Draw a green rectangle indicating the updated
		// area.
		HGDIOBJ sel;
		HPEN red = CreatePen(PS_SOLID,1,RGB(0,0xff,0));
		sel = SelectObject(w->bbuffer_hdc,red);
		MoveToEx(w->bbuffer_hdc, r->left, r->top, (LPPOINT) NULL);
		LineTo(w->bbuffer_hdc, r->left, r->bottom-1);
		LineTo(w->bbuffer_hdc, r->right-1, r->bottom-1);
		LineTo(w->bbuffer_hdc, r->right-1,r->top);
		LineTo(w->bbuffer_hdc, r->left,r->top);
		SelectObject(w->bbuffer_hdc,sel);
		DeleteObject(red);
	}
#endif
*/
}

extern void *window_invoke_msg_hooks(window *w,void *id1, void *id2,void* p1,void *p2);

static LRESULT WndProc( window *w, UINT msg, WPARAM wparam, LPARAM lparam) {
	// send message to registered hooks:
	LRESULT result = (LRESULT) window_invoke_msg_hooks(w,(void*)(intptr_t)msg,NULL,(void*)wparam,(void*)lparam);
	if (result) return result;
	// if the message was unhandled, do our own processing:
	switch( msg ) {
		case WM_CLOSE:
			if( !w->evt(w,WE_CLOSE,NULL) )
				// exit window close
				return 0;
			// continue window close
			break;

		case WM_NCDESTROY:
			// last incoming window message before window destruction
			// can not be cancelled
			result = DefWindowProc( w->hwnd, msg, wparam, lparam );
			ReleaseDC(w->hwnd,w->hdc);
			SetWindowLongPtr(w->hwnd,GWLP_USERDATA,0);
			w->hwnd = NULL;
			w->hdc = NULL;
			w->evt(w,WE_DESTROY,NULL);
			free(w);
			return result;

		case WM_ERASEBKGND:
			// return handled - otherwise DefWindowProc will blank out our client
			// area's background, causing flicker.
			return 1;

		case WM_PAINT: {
				PAINTSTRUCT ps;
				BeginPaint(w->hwnd,&ps);
				BitBlt	( ps.hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top //dest
						, w->bbuffer_hdc, ps.rcPaint.left, ps.rcPaint.top // source
						, SRCCOPY // rastermode
						);
				EndPaint(w->hwnd,&ps);
				return 1;
			}

		case WM_SETCURSOR: {
			RECT rc = { w->npwin.x, w->npwin.y, w->npwin.width, w->npwin.height };
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient( w->hwnd, &pt);

			if( PtInRect(&rc,pt) ) {
				sendEvent(w,msg,wparam,lparam);
				return 1;
			}
			break;
		}

		case WM_MOUSEWHEEL: {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient( w->hwnd, &pt);
			lparam = (pt.y << 16) | pt.x;
			sendEvent(w,msg,wparam,lparam);
			break;
		}

		case WM_RBUTTONDOWN:
			if (!w->evt(w,WE_RIGHTCLICK,NULL))
				return 1;
			else
				sendEvent(w,msg,wparam,lparam);
			break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
			SetCapture(w->hwnd); // allow releaseOutside
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MOUSEMOVE:
		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
			sendEvent(w, msg, wparam, lparam);
			break;

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_MOUSEACTIVATE:
		case WM_ACTIVATEAPP:
			sendEvent( w, msg, wparam, lparam );
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSCHAR:
		case WM_SYSDEADCHAR:
			if (GetFocus() == w->hwnd)
				sendEvent( w, msg, wparam, lparam );
			break;

		case WM_WINDOWPOSCHANGED: {
				WINDOWPOS * wp = (WINDOWPOS*) lparam;
				if (!(wp->flags & SWP_NOSIZE)) {
					RECT rc;
					GetClientRect(w->hwnd, &rc);
					updateFlashMetrics(w, rc.right-rc.left, rc.bottom - rc.top);
					applyFlashMetrics(w);
				}
				return 0;
		}

		case WM_SYSCOMMAND: {
			int internal = w->placement_int;
			w->placement_int = 0;
			switch(wparam & 0xFFF0) {
				case SC_MAXIMIZE:
					if (!internal && !w->evt(w,WE_MAXIMIZE,NULL)) return 0;
					if (!w->placement_org) {
						w->placement_org = malloc(sizeof(WINDOWPLACEMENT));
						w->placement_org->length = sizeof(WINDOWPLACEMENT);
						GetWindowPlacement(w->hwnd, w->placement_org);
					}
					break;
				case SC_MINIMIZE:
					if (!internal && !w->evt(w,WE_MINIMIZE,NULL))
						return 0;
					break;
				case SC_RESTORE:
					if (!internal)
						w->evt(w,WE_RESTORE,NULL);
					if (w->placement_org) {
						free(w->placement_org);
						w->placement_org = 0;
					}
					break;
			}
			break;
		}

		case WM_DROPFILES: {
			HDROP hdrop = (HDROP) wparam;
			int nfiles = DragQueryFile(hdrop,0xFFFFFFFF,0,0);
			if( nfiles > 0 ) {
				int i;
				string_list data;
				data.count = nfiles;
				data.strings = malloc(nfiles*sizeof(char*));
				for(i=0;i<nfiles;i++) {
					int bytes = DragQueryFile(hdrop,i,0,0) + 1;
					data.strings[i] = malloc(bytes);
					DragQueryFile(hdrop,i,data.strings[i],bytes);
				}
				DragFinish(hdrop);
				w->evt(w,WE_FILESDROPPED,&data);
				for(i=0;i<nfiles;i++)
					free(data.strings[i]);
				free(data.strings);
			}
			break;
		}
		default:
			break;
	}
	return DefWindowProc( w->hwnd, msg, wparam, lparam );
}

static LRESULT CALLBACK WndProcStub( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	window *w = (window*)getptr(hwnd,GWLP_USERDATA);
	if( w != NULL )
		return WndProc( w, msg, wparam, lparam );
	return DefWindowProc( hwnd, msg, wparam, lparam );
}

static void fullscreen(window *w, bool enter) {

	RECT rc;
	if (enter) {
		w->fullscreen_org_wi.cbSize = sizeof(WINDOWINFO);
		GetWindowInfo(w->hwnd,&w->fullscreen_org_wi);
		SetWindowLong(w->hwnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU );
		GetWindowRect(GetDesktopWindow(), &rc);
		SetWindowPos( w->hwnd, HWND_TOPMOST, 0, 0, rc.right - rc.left, rc.bottom - rc.top, 0);
	}
	else
	{
		RECT *rc = &w->fullscreen_org_wi.rcWindow;
		SetWindowLong( w->hwnd, GWL_STYLE, w->fullscreen_org_wi.dwStyle );
		SetWindowLong( w->hwnd, GWL_EXSTYLE, w->fullscreen_org_wi.dwExStyle );
		SetWindowPos
			( w->hwnd, HWND_NOTOPMOST
			, rc->left, rc->top
			, rc->right-rc->left, rc->bottom-rc->top
			, 0 );
	}
}

void system_launch_url( const char *url ) {
	ShellExecute(0,"open",url,0,0,SW_SHOW);
}
