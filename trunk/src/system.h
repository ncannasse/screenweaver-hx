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

#ifndef _SWHX_SYSTEM_H
#define _SWHX_SYSTEM_H

enum WindowEvent {
	WE_DESTROY,
	WE_CLOSE,
	WE_MINIMIZE,
	WE_MAXIMIZE,
	WE_RIGHTCLICK,
	WE_FILESDROPPED, // data = string_list
	WE_RESTORE
};

enum WindowProperty {
	WP_RESIZABLE,
	WP_MAXIMIZE_ICON,
	WP_MINIMIZE_ICON,
	WP_WIDTH,
	WP_HEIGHT,
	WP_LEFT,
	WP_TOP,
	WP_FULLSCREEN,
	WP_TRANSPARENT,
	WP_FLASH_RUNNING,
	WP_DROPTARGET,
	WP_PLAIN,
	WP_MINIMIZED,
	WP_MAXIMIZED
};

enum WindowFlags {
	WF_FULLSCREEN		= 1,
	WF_TRANSPARENT		= 1 << 1,
	WF_FLASH_RUNNING	= 1 << 2,
	WF_DROPTARGET		= 1 << 3,
	WF_PLAIN			= 1 << 4,
	WF_ALWAYS_ONTOP		= 1 << 5,
	WF_NO_TASKBAR		= 1 << 6
};

#include <npapi/npapi.h>

// data structures

typedef struct {
	int count;
	char** strings;
} string_list;

typedef struct _window window;
typedef struct _library library;
typedef struct _private_data private_data;
typedef int (*on_event)( window *w, enum WindowEvent e, void* );
typedef int (*on_npevent)( window *w, NPEvent *e );
typedef void (*gen_callback)( void * );

typedef struct _window_list {
	window *w;
	struct _window_list *next;
} window_list;

extern window_list *windows;

// MAIN functions
int system_init();
void system_cleanup();
void system_loop();
void system_loop_exit();

// DLL functions
library *system_library_open( const char *path );
void *system_library_symbol( library *l, const char *symbol );
void system_library_close( library *l );

// WINDOW functions
window *system_window_create( const char *title, int width, int height, enum WindowFlags flags, on_event f );
void system_window_show( window *w, int show );
void system_window_set_npevent( window *w, on_npevent f );
void system_window_set_private( window *w, private_data *p );
void system_window_destroy( window *w );

private_data *system_window_get_private( window *w );
void *system_window_get_handle( window *w );
NPWindow *system_window_getnp( window *w );
void system_window_invalidate( window *w, NPRect *r );
void system_window_set_flash( window *w, void *f );
void system_window_set_title( window *w, const char* title );
void system_window_drag( window *w );
void system_window_resize( window *w, int o );

void system_window_set_prop( window *w, enum WindowProperty prop, int value );
int system_window_get_prop( window *w, enum WindowProperty prop );

// MISC functions
void system_sync_call( gen_callback func, void *param );
int system_is_main_thread();
char *system_fullpath( const char *file );
char *system_plugin_file_version( const char *file );
void system_launch_url( const char *url );

#endif // _SWHX_SYSTEM_H
