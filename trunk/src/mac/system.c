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
#include "../system.h"
#include "../flash_dll.h"
#include "../flash.h"

#include <Carbon/Carbon.h>
#include "/usr/include/dlfcn.h"
#include <npapi/npapi.h>
#include <pthread.h>

window_list *windows = NULL;
extern flash_dll *fl_dll;

// --------------------------------------- TOOLS -----------------------------------------

void system_launch_url( const char *url ) {
	ICInstance icInstance;
	OSType psiSignature = 'Psi ';
    OSStatus error = ICStart( &icInstance, psiSignature );
    if ( error == noErr ) {
		ConstStr255Param hint = 0x0;
		const char* data = url;
		long length = strlen(url);
		long start = 0;
		long end = length;
		ICLaunchURL( icInstance, hint, data, length, &start, &end );
		ICStop( icInstance );
	}
}

// -------------------------------------- PLUGIN -----------------------------------------

char *system_fullpath( const char *file ) {
	char path[2048];
	FSRef ref;
	if (FSPathMakeRef ((const UInt8*) file, &ref, 0 )!=0)
		return strdup(path);
	if (FSRefMakePath ( &ref, (UInt8*) path, 2048 )!=0)
		return strdup(path);
	char prefixed[2048];
	strcpy(prefixed,"file:///");
	strcat(prefixed,path);
	return strdup(prefixed);
}

char *system_plugin_file_version( const char *file ) {
	char* result = NULL;
	FSRef ref;
	CFURLRef url;
	CFBundleRef bundle;

	if (FSPathMakeRef ((const UInt8*) file, &ref, 0 ) == noErr) {
		url = CFURLCreateFromFSRef(NULL,&ref);
		bundle = CFBundleCreate(NULL,url);
		if (bundle) {
			CFTypeRef data = CFBundleGetValueForInfoDictionaryKey(bundle,CFSTR("CFBundleShortVersionString"));
			if (CFGetTypeID(data) == CFGetTypeID(CFSTR(""))) {
				CFStringRef version = (CFStringRef) data;
				CFIndex length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(version),kCFStringEncodingUTF8);
				result = malloc(length+1);
				CFStringGetCString(version,result,length,kCFStringEncodingUTF8);
				result[length] = 0;
			}
			CFRelease(bundle);
		}
	}
	return result;
}

// -------------------------------------- LIBRARY -----------------------------------------

library *system_library_open( const char *path ) {
	printf("loading lib: [%s]",path);
	return (void*) dlopen( path, RTLD_LAZY );
}

void *system_library_symbol( library *l, const char *symbol ) {
	return (library*)dlsym( (void*)l, symbol );
}

void system_library_close( library *l ) {
	dlclose( (void*)l );
}

// -------------------------------------- SYSTEM -----------------------------------------

int system_init() {
	return 0;
}

void system_cleanup() {
}

// -------------------------------------- WINDOW -----------------------------------------

struct _window {
	WindowRef ref;
	NPWindow npwin;
	NP_Port npport;
	void *flash;
	on_event evt;
	on_npevent npevent;
	private_data *p;

	EventLoopTimerRef timer;
	enum WindowFlags flags;
	Rect fullscreen_org_rc;
	WindowAttributes fullscreen_attr;
	Point window_org_size;

	GWorldPtr gw;
	CGContextRef ctx;
	unsigned char* bits;

	GWorldPtr gw_b;
	CGContextRef ctx_b;
	unsigned char* bits_b;

	msg_hook_list *msg_hooks;
};

EventRecord nullEventRec = {0,0,0,0,0};

static void setupHandlers(window *w);
static void timerStub(EventLoopTimerRef theTimer, EventLoopIdleTimerMessage msg, void* userData);
static void updateFlashMetrics(window *, int width, int height);
static void applyFlashMetrics(window *w);
static void fullscreen(window *w, short enter);
static void reallocateOffscreenBuffer(window *w);
static void freeOffscreenBuffer(window *w);

window *system_window_create( const char *title, int width, int height, enum WindowFlags flags, on_event f ){
	OSStatus status;
	WindowRef ref;
	window *w;
	Rect rc = {0,0,height,width};
	CFStringRef cftitle = CFStringCreateWithCString(0,title,kCFStringEncodingUTF8);
	WindowClass winclass;
	WindowAttributes winattr = kWindowStandardHandlerAttribute;
	if (flags & WF_PLAIN) {
		// plain window
		winclass = kPlainWindowClass;
	} else if (flags & WF_TRANSPARENT) {
		// transparent window
		winclass = kOverlayWindowClass;
	} else {
		// regular OS chrome window
		winclass = kDocumentWindowClass;
		winattr |= kWindowStandardDocumentAttributes;
	}

	status = CreateNewWindow( winclass, winattr, &rc, &ref );

	HIWindowChangeFeatures
		( ref
		, kWindowCanCollapse
		| kWindowCanZoom
		, 0
		);

	w = malloc(sizeof(struct _window));
	memset(w,0,sizeof(struct _window));
	w->ref = ref;
	w->evt = f;
	w->flags = flags ^ WF_FLASH_RUNNING;
	w->window_org_size.h = width;
	w->window_org_size.v = height;

	RepositionWindow(w->ref,NULL,kWindowCascadeOnMainScreen);
	IsWindowInStandardState(w->ref,&w->window_org_size,NULL);

	w->npport.port = GetWindowPort(w->ref);
	w->npwin.window = &w->npport;
	w->npwin.type = NPWindowTypeDrawable;

	updateFlashMetrics(w, width, height);
	if (w->flags & WF_TRANSPARENT) {
		reallocateOffscreenBuffer(w);
		SetWindowActivationScope(ref, kWindowActivationScopeAll);
	}
	SetWindowTitleWithCFString (ref, cftitle);

	setupHandlers(w);

	InstallEventLoopIdleTimer
		( GetMainEventLoop()
		, 0.000000000001
		, 0.02 // 50 fps. max?
		, NewEventLoopIdleTimerUPP(timerStub)
		, w
		, &w->timer
		);

	return w;
}

void updateFlashMetrics(window *w, int width, int height) {
	w->npwin.clipRect.bottom = height;
	w->npwin.clipRect.right = width;
	w->npwin.height = height;
	w->npwin.width = width;
}

void applyFlashMetrics(window *w) {
	if (w->flags && WF_TRANSPARENT)
		reallocateOffscreenBuffer(w);
	if (w->flash) {
		fl_dll->table.setwindow(flashp_get_npp(w->flash),&w->npwin);
		system_window_invalidate(w,&w->npwin.clipRect);
	}
}

void system_window_show( window *w, int show ) {
	if (show) {
		ShowWindow(w->ref);
		ActivateWindow(w->ref,true);
		if( w->flags & WF_TRANSPARENT && !(w->flags & WF_ALWAYS_ONTOP) )
			SetWindowGroup(w->ref, GetWindowGroupOfClass(kDocumentWindowClass));
	}
	else
		HideWindow(w->ref);
}

void system_window_destroy( window *w ) {
	freeOffscreenBuffer(w);
	DisposeWindow(w->ref);
}

void system_window_destroy_take_void(void *w) {
	system_window_destroy((window *)w);
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
	return w->ref;
}

void system_window_set_flash( window *w, void *f ) {
	w->flash = f;
}

void system_window_invalidate( window *w, NPRect *r ) {
	Rect rc = {r->left, r->top, r->bottom, r->right};
	InvalWindowRect(w->ref, &rc);
}

static bool sendEvent( window *w, EventRecord *er )
{
	if( w->npevent == NULL )
		return 0;
	else
		return w->npevent(w,er);
}

static void timer(window *w, EventLoopTimerRef theTimer, EventLoopIdleTimerMessage msg) {
	nullEventRec.when = GetCurrentEventTime();
	GetMouse(&nullEventRec.where);
	sendEvent(w, &nullEventRec);
}

static void timerStub(EventLoopTimerRef theTimer, EventLoopIdleTimerMessage msg, void* userData) {
	window *w = (window*) userData;
	timer(w, theTimer, msg);
}

#define SET(add,rem,flags,toggle) toggle ? (add |= flags) : (rem |= flags)
#define IS_SET(v,flags)	(((v) & flags) == flags)

void system_window_set_prop( window *w, enum WindowProperty prop, int value ) {
	WindowAttributes add = 0;
	WindowAttributes rem = 0;

	switch( prop ) {
		case WP_RESIZABLE:
			SET(add,rem,kWindowResizableAttribute,value);
			break;
		case WP_MAXIMIZE_ICON:
			SET(add,rem,kWindowHorizontalZoomAttribute|kWindowVerticalZoomAttribute,value);
			break;
		case WP_MINIMIZE_ICON:
			SET(add,rem,kWindowCollapseBoxAttribute,value);
			break;
		case WP_WIDTH:
		case WP_HEIGHT:	{
			Rect rc;
			GetWindowBounds(w->ref,kWindowContentRgn,&rc);
			if( prop == WP_WIDTH )
				rc.right = value;
			else
				rc.bottom = value;
			updateFlashMetrics(w,rc.right,rc.bottom);
			applyFlashMetrics(w);
			SetWindowBounds(w->ref,kWindowContentRgn,&rc);
			break;
		}
		case WP_TOP:
		case WP_LEFT: {
			Rect rc;
			int offset;
			GetWindowBounds(w->ref,kWindowStructureRgn,&rc);
			if ( prop==WP_TOP ) {
				offset = value - rc.top;
				rc.top = value;
				rc.bottom += offset;
			} else {
				offset = value - rc.left;
				rc.left = value;
				rc.right += offset;
			}
			SetWindowBounds(w->ref,kWindowStructureRgn,&rc);
			break;
		}
		case WP_FULLSCREEN:
			if (w->flags & WF_FULLSCREEN) {
				if (value == 0) {
					fullscreen(w, 0);
					w->flags &= ~WF_FULLSCREEN;
				}
			} else {
				if (value) {
					fullscreen(w, 1);
					w->flags |= WF_FULLSCREEN;
				}
			}
			return;
			break;
		case WP_FLASH_RUNNING:
			if (!value)
				w->flags &= ~WF_FLASH_RUNNING;
			else {
				w->flags |= WF_FLASH_RUNNING;
				applyFlashMetrics(w);
			}
			break;
		case WP_DROPTARGET:
			if (!value)
				w->flags &= ~WF_DROPTARGET;
			else
				w->flags |= WF_DROPTARGET;
			break;
		case WP_MAXIMIZED: {
			Point pt;
			Boolean stdstate = IsWindowInStandardState(w->ref,&w->window_org_size,NULL);
			if (!value && !stdstate)
				ZoomWindowIdeal(w->ref,inZoomIn,&pt);
			else if (value && stdstate) {
				Rect bounds;
				GetWindowGreatestAreaDevice(w->ref,kWindowStructureRgn,NULL,&bounds);
				pt.h = bounds.right - bounds.left;
				pt.v = bounds.bottom - bounds.top;
				ZoomWindowIdeal(w->ref,inZoomOut,&pt);
			}
			break;
		}
		case WP_MINIMIZED:
			if (!value && IsWindowCollapsed(w->ref))
				CollapseWindow(w->ref,true);
			else if (value && !IsWindowCollapsed(w->ref))
				CollapseWindow(w->ref,true);
			break;
	}
	if (add|rem) ChangeWindowAttributes(w->ref,add,rem);
}

int system_window_get_prop( window *w, enum WindowProperty prop ) {
	WindowAttributes a;
	GetWindowAttributes(w->ref, &a);

	switch( prop ) {
		case WP_RESIZABLE:
			return IS_SET(a,kWindowResizableAttribute);
		case WP_MAXIMIZE_ICON:
			return IS_SET(a,kWindowHorizontalZoomAttribute|kWindowVerticalZoomAttribute);
		case WP_MINIMIZE_ICON:
			return IS_SET(a,kWindowCollapseBoxAttribute);
		case WP_WIDTH:
		case WP_HEIGHT: {
			Rect rc;
			GetWindowBounds(w->ref,kWindowContentRgn,&rc);
			if( prop==WP_HEIGHT )
				return w->window_org_size.v = rc.bottom-rc.top;
			else
				return w->window_org_size.h = rc.right-rc.left;
		}
		case WP_TOP:
		case WP_LEFT: {
			Rect rc;
			GetWindowBounds(w->ref,kWindowStructureRgn,&rc);
			if ( prop==WP_TOP )
				return rc.top;
			else
				return rc.left;
			break;
		}
		case WP_FULLSCREEN:
			return w->flags & WF_FULLSCREEN;
		case WP_FLASH_RUNNING:
			return w->flags & WF_FLASH_RUNNING;
		case WP_TRANSPARENT:
			return w->flags & WF_TRANSPARENT;
		case WP_DROPTARGET:
			return w->flags & WF_DROPTARGET;
		case WP_PLAIN:
			return w->flags & WF_PLAIN;
		case WP_MAXIMIZED:
			return IsWindowInStandardState(w->ref,&w->window_org_size,NULL) ? 0 : 1;
		case WP_MINIMIZED:
			return IsWindowCollapsed(w->ref) ? 1 : 0;
	}
	return 0;
}

void system_window_set_title( window *w, const char *title ) {
	CFStringRef cftitle = CFStringCreateWithCString(0,title,kCFStringEncodingUTF8);
	SetWindowTitleWithCFString(w->ref,cftitle);
}

void system_window_drag( window *w ) {
	Point pt;
	GetGlobalMouse(&pt);
	DragWindow(w->ref,pt,NULL);
}

void system_window_resize( window *w, int o ) {
	Point pt;
	Rect rc;
	GetGlobalMouse(&pt);
	ResizeWindow(w->ref,pt,NULL,&rc);
}

msg_hook_list **system_window_get_msg_hook_list( window *w ) {
	return &w->msg_hooks;
}

#define INSTALL_HANDLER(w,c,e,handler) {EventTypeSpec ets = {c,e}; InstallWindowEventHandler(w->ref,NewEventHandlerUPP(handler),1,&ets,w,NULL);}
#define INSTALL_HANDLERS(w,e,handler) InstallWindowEventHandler(w->ref,NewEventHandlerUPP(handler),GetEventTypeCount(e),e,w,NULL);

#define GET_WINDOW window*w = u
#define HANDLED return noErr
#define UNHANDLED return CallNextEventHandler(nextHandler,theEvent)

/* Custom events, listed in npapi.h:

NPEventType_GetFocusEvent = (osEvt + 16),
NPEventType_LoseFocusEvent,
NPEventType_AdjustCursorEvent,
NPEventType_MenuCommandEvent,
NPEventType_ClippingChangedEvent,
NPEventType_ScrollingBeginsEvent = 1000,
NPEventType_ScrollingEndsEvent

*/

static OSStatus onClose(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onClosed(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onActivated(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onDeactivated(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onMinimize(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onMaximize(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onRestore(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onShown(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onUpdate(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onUpdate_transparent(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onBoundsChanging(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onMouse(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);
static OSStatus onKeyboard(EventHandlerCallRef nextHandler, EventRef theEvent, void* u);

static OSErr onDragReceive(WindowRef theWindow,void * handlerRefCon,DragRef theDrag);
static OSErr onDragTracking(DragTrackingMessage message,WindowRef theWindow,void * handlerRefCon,DragRef theDrag);

static OSErr aeOpenDocuments(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);

void setupHandlers(window *w) {
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowClose, onClose);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowClosed, onClosed);

	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowActivated, onActivated);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowDeactivated, onDeactivated);

	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowCollapse, onMinimize);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowZoom, onMaximize);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowExpand, onRestore);

	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowShown, onShown);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowUpdate, (w->flags & WF_TRANSPARENT) ? onUpdate_transparent : onUpdate);
	INSTALL_HANDLER(w, kEventClassWindow, kEventWindowBoundsChanging, onBoundsChanging);

	EventTypeSpec mouseevts[] =
		{{kEventClassMouse, kEventMouseMoved}
		,{kEventClassMouse, kEventMouseDown}
		,{kEventClassMouse, kEventMouseUp}
		,{kEventClassMouse, kEventMouseWheelMoved}
		,{kEventClassMouse, kEventMouseWheelAxisX}
		,{kEventClassMouse, kEventMouseWheelAxisY}
		,{kEventClassWindow, kEventWindowCursorChange}
		};
	INSTALL_HANDLERS(w, mouseevts, onMouse);

	EventTypeSpec kbevts[] =
		{{kEventClassKeyboard, kEventRawKeyDown}
		,{kEventClassKeyboard, kEventRawKeyRepeat}
		,{kEventClassKeyboard, kEventRawKeyUp}
		};
	INSTALL_HANDLERS(w, kbevts, onKeyboard);

	InstallReceiveHandler(NewDragReceiveHandlerUPP(onDragReceive),w->ref,w);
	InstallTrackingHandler(NewDragTrackingHandlerUPP(onDragTracking),w->ref,w);

	// TODO: handle the standard AppleEvents,
	// (making this opaque over platforms will be a pickle)
	//AEInstallEventHandler(kCoreEventClass,kAEOpenApplication,NewAEEventHandlerUPP(OpenApplicationAE),0,false);
	//AEInstallEventHandler(kCoreEventClass,kAEReopenApplication,NewAEEventHandlerUPP(ReopenApplicationAE),0,false);
    //AEInstallEventHandler(kCoreEventClass,kAEOpenDocuments,NewAEEventHandlerUPP(aeOpenDocuments),0,false);

}

static OSStatus onClose(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	if( w->evt(w,WE_CLOSE,NULL) ) {
		RemoveEventLoopTimer(w->timer);
		DisposeWindow(w->ref);
		UNHANDLED;
	} else HANDLED;
}

static OSStatus onClosed(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	w->evt(w,WE_DESTROY,NULL);
	free(w);
	UNHANDLED;
}

static OSStatus onActivated(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	EventRecord er;
	ConvertEventRefToEventRecord(theEvent, &er);
	sendEvent(w, &er);
	er.what = NPEventType_GetFocusEvent;
	sendEvent(w, &er);
	UNHANDLED;
}

static OSStatus onDeactivated(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	EventRecord er;
	ConvertEventRefToEventRecord(theEvent, &er);
	sendEvent(w, &er);
	er.what = NPEventType_LoseFocusEvent;
	sendEvent(w, &er);
	UNHANDLED;
}

static OSStatus onMinimize(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	if (w->evt(w,WE_MINIMIZE,NULL))
		UNHANDLED;
	else
		HANDLED;
}

static OSStatus onMaximize(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	if (w->evt(w,WE_MAXIMIZE,NULL))
		UNHANDLED;
	else
		HANDLED;
}

static OSStatus onRestore(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	w->evt(w,WE_RESTORE,NULL);
	UNHANDLED;
}

static OSStatus onShown(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	Rect rc;
	GetWindowBounds(w->ref, kWindowContentRgn, &rc );
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	rc.top = 0;
	rc.left = 0;
	InvalWindowRect(w->ref,&rc);
	UNHANDLED;
}

static OSStatus onUpdate(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	EventRecord er;

	// regular drawing:
	ConvertEventRefToEventRecord(theEvent, &er);
	BeginUpdate(w->ref);
	sendEvent(w,&er);
	EndUpdate(w->ref);

	HANDLED;
}

static OSStatus onBoundsChanging(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	UInt32 attributes;
	GetEventParameter(theEvent,kEventParamAttributes,typeUInt32,NULL,sizeof(UInt32),NULL,&attributes);
	if (attributes & kWindowBoundsChangeSizeChanged) {
		Rect bounds;
		UInt32 width, height;
		GetEventParameter(theEvent,kEventParamCurrentBounds,typeQDRectangle,NULL,sizeof(Rect),NULL,&bounds);
		width = bounds.right - bounds.left;
		height = bounds.bottom - bounds.top;
		updateFlashMetrics(w, width, height);
		applyFlashMetrics(w);
		if (attributes & kWindowBoundsChangeUserDrag || attributes & kWindowBoundsChangeUserResize) {
			w->window_org_size.h = width;
			w->window_org_size.v = height;
		}
	}
	UNHANDLED;
}

static OSStatus onMouse(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	WindowPartCode part;
	GetEventParameter(theEvent, kEventParamWindowPartCode, typeWindowPartCode, 0, sizeof(WindowPartCode), 0, &part);
	if (part == inContent) {
		EventRecord er;
		if (GetEventKind(theEvent) == kEventMouseDown) {
			EventMouseButton mb;
			UInt32 km;
			GetEventParameter(theEvent,kEventParamMouseButton,typeMouseButton,NULL,sizeof(EventMouseButton),NULL,&mb);
			GetEventParameter(theEvent,kEventParamKeyModifiers,typeUInt32,NULL,sizeof(UInt32),NULL,&km);
			if (mb==kEventMouseButtonSecondary || km & controlKey) {
				if (!w->evt(w,WE_RIGHTCLICK,NULL))
					UNHANDLED;
			}
		}
		if (!ConvertEventRefToEventRecord(theEvent, &er)) {
			/*
			 * Need to figure out how to pass a mouse-wheel event to the Flash plugin. The NP-API uses
			 * an old event model that doesn't support the mouse wheel.
			 *
			er.message = ;
			er.modifiers = ;
			er.what = ;
			er.when = ;
			er.where = ;
			*/
		}
		sendEvent(w,&er);
	}
	if (IsWindowCollapsed(w->ref))
		HANDLED;
	else
		UNHANDLED;
}

static OSStatus onKeyboard(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	EventRecord er;
	ConvertEventRefToEventRecord(theEvent, &er);
	sendEvent(w,&er);
	HANDLED;
}

static void fullscreen(window *w, short enter) {
	static WindowAttributes setAttr = (1L << 9); //== kWindowNoTitleBarAttribute. TODO: This might not work on 10.3.9
	static WindowAttributes remAttr = 0;

	Rect rcwin;
	GetWindowBounds(w->ref, kWindowStructureRgn, &rcwin );
	if (enter) {
		// enter fullscreen
		CGPoint pt = {rcwin.left,rcwin.top};
		CGRect rc;

		CGDirectDisplayID id;
		CGDisplayCount dc;

		SetSystemUIMode
			( kUIModeAllHidden
			, kUIOptionDisableAppleMenu
			| kUIOptionDisableProcessSwitch
            | kUIOptionDisableForceQuit
			);
		CGGetDisplaysWithPoint(pt,1,&id,&dc);
		rc = CGDisplayBounds(id);

		memcpy(&w->fullscreen_org_rc,&rcwin,sizeof(Rect));
		rcwin.left = rc.origin.x;
		rcwin.top = rc.origin.y;
		rcwin.right = rc.size.width + rcwin.left;
		rcwin.bottom = rc.size.height + rcwin.top;

		GetWindowAttributes(w->ref, &w->fullscreen_attr);
		ChangeWindowAttributes(w->ref, setAttr, remAttr );
		SetWindowBounds(w->ref,kWindowStructureRgn,&rcwin);
	} else {
		// leave fullscreen
		SetSystemUIMode(kUIModeNormal, nil);
		ChangeWindowAttributes(w->ref,w->fullscreen_attr, setAttr & ~w->fullscreen_attr);
		SetWindowBounds(w->ref,kWindowStructureRgn,&w->fullscreen_org_rc);
	}
}

OSErr onDragReceive(WindowRef theWindow,void * handlerRefCon,DragRef theDrag) {
	window *w = (window*) handlerRefCon;
	if (w->flags & WF_DROPTARGET) {
		string_list sl = {0,0};
		UInt16 itemcnt;
		CountDragItems(theDrag,&itemcnt);
		while(itemcnt>0) {
			DragItemRef itemref;
			UInt16 flavorcnt;
			GetDragItemReferenceNumber(theDrag,itemcnt--,&itemref);
			CountDragItemFlavors(theDrag,itemref,&flavorcnt);
			while(flavorcnt>0) {
				FlavorType flavor;
				GetFlavorType(theDrag,itemref,--flavorcnt,&flavor);
				if (flavor == typeFileURL) {
					Size size;
					sl.strings = realloc(sl.strings,(sl.count+1)*(sizeof(char*)));
					GetFlavorDataSize(theDrag,itemref,typeFileURL,&size);
					sl.strings[sl.count] = malloc(size+1);
					GetFlavorData(theDrag,itemref,typeFileURL,sl.strings[sl.count],&size,0);
					sl.strings[sl.count][size] = 0;
					if(strncmp(sl.strings[sl.count], "file://localhost", 16) == 0) {
						memmove(sl.strings[sl.count],sl.strings[sl.count]+16,size-16);
						sl.strings[sl.count][size-16]=0;
					}
					sl.count++;
				}
			}
		}
		if (sl.count) {
			w->evt(w,WE_FILESDROPPED,&sl);
		}
	}
	return 0;
}

OSErr onDragTracking(DragTrackingMessage message,WindowRef theWindow,void * handlerRefCon,DragRef theDrag) {
	window *w = (window*) handlerRefCon;
	if (w->flags & WF_DROPTARGET) {
		//TODO: Give visual feedback
	}
	return 1;
}

static OSErr aeOpenDocuments(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon) {
    AEDescList  docList;
    FSRef       theFSRef;
    long        index;
    long        count = 0;
    OSErr       err = AEGetParamDesc(theAppleEvent,keyDirectObject, typeAEList, &docList);
    require_noerr(err, CantGetDocList);

    err = AECountItems(&docList, &count);
    require_noerr(err, CantGetCount);

    for(index = 1; index <= count; index++) {
        err = AEGetNthPtr(&docList,index,typeFSRef,NULL,NULL,&theFSRef,sizeof(FSRef),NULL);
		require_noerr(err, CantGetDocDescPtr);
    }
    AEDisposeDesc(&docList);

CantGetDocList:
CantGetCount:
CantGetDocDescPtr:
    if (err != noErr)  {
        // For handlers that expect a reply, add error information here.
    }
    return(err);
}

/* ----------- Transparent Windowing -------

The Netscape plug-in API uses old-style Mac QuickTime
drawing. Transparent Windows use Core Graphics though.

To get the two (QT and CG) working together we have
them share the bits to an off-screen buffer.

*/

static void freeOffscreenBuffer(window *w) {
	if (w->bits) {
		CGContextRelease(w->ctx);
		DisposeGWorld(w->gw);
		free(w->bits);
		CGContextRelease(w->ctx_b);
		DisposeGWorld(w->gw_b);
		free(w->bits_b);
		w->bits = NULL;
		w->bits_b = NULL;
	}
}

static void reallocateOffscreenBuffer(window *w) {
	Rect rc;
	size_t wi,h,r;
	// free prev. allocated buffers:
	freeOffscreenBuffer(w);
	// get current window metrics:
	rc.left = 0;
	rc.top = 0;
	rc.right = w->npwin.clipRect.right;
	rc.bottom = w->npwin.clipRect.bottom;
	wi = rc.right;
	h = rc.bottom;
	r = 4*wi;
	// main buffer;
	w->bits = malloc(h*r);
	w->ctx = CGBitmapContextCreate(w->bits,wi,h,8,r,CGColorSpaceCreateDeviceRGB(),kCGImageAlphaPremultipliedFirst);
	QTNewGWorldFromPtr(&w->gw,k32ARGBPixelFormat,&rc,NULL,NULL,kNativeEndianPixMap,w->bits,r);
	// secundary buffer:
	w->bits_b = malloc(h*r);
	w->ctx_b = CGBitmapContextCreate(w->bits_b,wi,h,8,r,CGColorSpaceCreateDeviceRGB(),kCGImageAlphaPremultipliedFirst);
	QTNewGWorldFromPtr(&w->gw_b,k32ARGBPixelFormat,&rc,NULL,NULL,kNativeEndianPixMap,w->bits_b,r);
}

/*
Transparent rendering: Must be optimized.
*/
static OSStatus onUpdate_transparent(EventHandlerCallRef nextHandler, EventRef theEvent, void* u) {
	GET_WINDOW;
	EventRecord er;

	ConvertEventRefToEventRecord(theEvent, &er);

	CGContextRef cgctx;
	CGImageRef ir;
	Rect rc;
	CGRect grc;
	CGrafPtr port = w->npport.port;
	long buffersize,i;

	// NOTE: currently ingnoring the actual update region & blitting the whole image.
	// Get current window metrics;
	GetWindowBounds(w->ref, kWindowStructureRgn, &rc );
	// Prepare core graphics rectangle:
	grc.origin.x = 0;
	grc.origin.y = 0;
	grc.size.width = rc.right-rc.left;
	grc.size.height = rc.bottom-rc.top;

	// prepare the back-buffers:
	buffersize = grc.size.width*grc.size.height;
	for (i=0; i<buffersize; i++) {
		((long*)w->bits)[i]=0xFFFFFFFF;		// render on white
		((long*)w->bits_b)[i]=0x000000FF;	// render on black
	}

	// paint image in our main off-screen port/buffer:
	w->npport.port = w->gw;
	sendEvent(w,&er);

	// paint image in our secundary off-screen port/buffer;
	w->npport.port = w->gw_b;
	sendEvent(w,&er);

	// recalculate aplha values:
	// NOTE: again, we're doing this disregarding the actual update region
	// but for the entire canvas...
	buffersize = buffersize * 4;
	for (i=0; i<buffersize; i += 4) {
		// pixel alpha value is the inverse of the difference between
		// one R,G, or B component:
		unsigned char a = ~(w->bits[i+1]-w->bits_b[i+1]);
		w->bits[i] = a;
		w->bits[i+1] = (w->bits[i+1] * a) >> 8;
		w->bits[i+2] = (w->bits[i+2] * a) >> 8;
		w->bits[i+3] = (w->bits[i+3] * a) >> 8;
	}

	// restore window port;
	w->npport.port = port;

	// copy image to screen:
	port = GetWindowPort(w->ref);
	QDBeginCGContext(port,&cgctx);
	ir = (CGImageRef) CGBitmapContextCreateImage((CGContextRef) w->ctx);
	CGContextClearRect(cgctx,grc);
	CGContextDrawImage(cgctx,grc,ir);
	CGImageRelease(ir);
	QDEndCGContext(port,&cgctx);

	HANDLED;
}
