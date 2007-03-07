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
#ifdef NEKO_INSTALLER
#	include <neko.h>
#	include <neko_vm.h>
#else
#	include <neko/neko.h>
#	include <neko/neko_vm.h>
#endif
#include "system.h"
#include "flash_dll.h"
#include "flash.h"

#ifdef NEKO_LINUX
#	include <stdlib.h> 
#	include <string.h>
#endif

/*
 * Stream syncing is on for Windows: fixes bug reported by Hosey:
 * http://lists.motion-twin.com/pipermail/haxe/2006-November/005795.html
 *
 * Stream syncing is off for Linux until sync calling is implemented in system.c
 */
#ifndef NEKO_LINUX
#define STREAM_SYNC
#endif

#ifdef STREAM_SYNC
#	define alloc_stream()	((stream*)malloc(sizeof(stream)))
#	define free_stream(s)	free(s)
#else
#	define alloc_stream()	((stream*)alloc_private(sizeof(stream)))
#	define free_stream(s)
#endif

#define val_window(x) ((window*)val_data(x))
#define val_window_msg_hook(x) ((window_msg_hook*)val_data(x))
#define val_window_msg_cb(x) ((msg_hook_callback)val_data(x))
#define val_flash(x) ((flash*)val_data(x))
#define val_stream(x) ((stream*)val_data(x))

DEFINE_KIND(k_window);
DEFINE_KIND(k_window_handle);
DEFINE_KIND(k_window_msg_hook);
DEFINE_KIND(k_window_msg_cb);
DEFINE_KIND(k_flash);
DEFINE_KIND(k_stream);

struct _private_data {
	value ondestroy;
	value onclose;
	value onminimize;
	value onmaximize;
	value onrightclick;
	value onfilesdropped;
	value onrestore;
	flash *player;
	value vplayer;
	value vwindow;
};

typedef struct _window_msg_hook {
	void *id1;				// message id to listen for
	void *id2;
	void *p1;				// params
	void *p2;
	msg_hook_callback fc;	// C to C callback
	value fn;				// Neko callback
};

typedef struct _msg_hook_list {
	window_msg_hook *hook;
	struct _msg_hook_list *next;
};

flash_dll *fl_dll = NULL;
static int in_call = 0;

#ifdef _MSC_VER
#	include <crtdbg.h>
#else
#	define _CrtSetDbgFlag(x)
#endif

extern void freeNPIds(void);


/* ************************************************************************* */
// APPLICATION

static value initialize( value path ) {
	const char *error;
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	val_check(path,string);
	fl_dll = dll_init(val_string(path),&error);
	if( fl_dll == NULL )
		val_throw( alloc_string(error) );
	if( system_init() )
		neko_error();
	return val_null;
}

static value loop() {
	system_loop();
	return val_null;
}

static value loop_exit() {
	system_loop_exit();
	return val_null;
}

static value cleanup() {
	while( windows != NULL )
		system_window_destroy(windows->w);
	dll_close(fl_dll);
	fl_dll = NULL;
	system_cleanup();
	freeNPIds();
	return val_null;
}

static value plugin_file_version( value path ) {
	char *info;
	val_check(path,string);
	info = system_plugin_file_version(val_string(path));
	if( info != NULL ) {
		value v = alloc_string(info);
		free(info);
		return v;
	}
	return val_null;
}

static void on_sync_call( void *k ) {
	value v = *(value*)k;
	free_root((value*)k);
	val_call0(v);
}

static value sync_call( value f ) {
	value *k;
	val_check_function(f,0);
	k = alloc_root(1);
	*k = f;
	system_sync_call(on_sync_call,k);
	return val_null;
}

static value is_main_thread() {
	return alloc_bool(system_is_main_thread());
}

/* ************************************************************************* */
// WINDOW

static int window_on_event( window *w, enum WindowEvent e, void *data ) {
	private_data *p = system_window_get_private(w);
	value exc = NULL;
	if( p == NULL )
		return 0;
	switch( e ) {
	case WE_CLOSE:
		if( p->onclose )
			return val_bool(val_call0(p->onclose));
		break;
	case WE_DESTROY:
		if( p->player != NULL ) {
			flashp_destroy(p->player);
			val_kind(p->vplayer) = NULL;
		}
		val_kind(p->vwindow) = NULL;
		if( p->ondestroy )
			val_callEx(val_null,p->ondestroy,NULL,0,&exc);
		free_root((value*)p);
		// remove from the window list
		{
			window_list *l = windows, *prev = NULL;
			while( l != NULL ) {
				if( l->w == w ) {
					if( prev == NULL )
						windows = l->next;
					else
						prev->next = l->next;
					free(l);
					break;
				}
				l = l->next;
			}
			// should never happen, but in case...
			if( l == NULL )
				val_throw( alloc_string("WINDOW NOT FOUND") );
		}
		// if there was an exception, throw it
		if( exc != NULL )
			val_rethrow(exc);
		break;
	case WE_MINIMIZE:
		if( p->onminimize )
			return val_bool(val_call0(p->onminimize));
		break;
	case WE_MAXIMIZE:
		if( p->onmaximize )
			return val_bool(val_call0(p->onmaximize));
		break;
	case WE_RIGHTCLICK:
		if( p->onrightclick )
			return val_bool(val_call0(p->onrightclick));
		break;
	case WE_FILESDROPPED:
		if( p->onfilesdropped ) {
			string_list *sl = (string_list*)data;
			value arg = alloc_array(sl->count);
			value r;
			int i;
			for(i=0;i<sl->count;i++)
				val_array_ptr(arg)[i] = alloc_string(sl->strings[i]);
			r = val_call1(p->onfilesdropped,arg);
			return val_bool(r);
		}
	case WE_RESTORE:
		if( p->onrestore )
			return val_bool(val_call0(p->onrestore));
		break;
	default:
		break;
	}
	return 0;
}

static int window_on_npevent( window *w, NPEvent *e ) {
	private_data *p = system_window_get_private(w);
	if( p->player == NULL )
		return 0;
	return fl_dll->table.event(flashp_get_npp(p->player),e);
}

static value window_create( value title, value width, value height, value flags ) {
	window *w;
	private_data *p;
	val_check(title,string);
	val_check(width,int);
	val_check(height,int);
	val_check(flags,int);
	if( fl_dll == NULL )
		neko_error();
	w = system_window_create(val_string(title),val_int(width),val_int(height),val_int(flags),window_on_event);
	if( w == NULL )
		neko_error();
	p = (private_data*)alloc_root(sizeof(struct _private_data)/sizeof(value));
	memset(p,0,sizeof(struct _private_data));
	p->vwindow = alloc_abstract(k_window,w);
	system_window_set_private(w,p);
	system_window_set_npevent(w,window_on_npevent);
	// add the window to the list
	{
		window_list *wl = (window_list*)malloc(sizeof(struct _window_list));
		wl->w = w;
		wl->next = windows;
		windows = wl;
	}
	return p->vwindow;
}

static value window_show( value w, value b ) {
	val_check_kind(w,k_window);
	val_check(b,bool);
	system_window_show(val_window(w),val_bool(b));
	return val_null;
}

// Macro helper for defining window events
#define SET_WIN_EVENT_HANDLER(field,argcnt)\
static value window_on_##field( value w, value f ) {\
	val_check_kind(w,k_window);\
	if( !val_is_null(f) )\
		val_check_function(f,argcnt);\
	system_window_get_private(val_window(w))->on##field = val_is_null(f)?NULL:f;\
	return val_null;\
}\
DEFINE_PRIM(window_on_##field,2)
// EO macro

SET_WIN_EVENT_HANDLER(destroy,0)
SET_WIN_EVENT_HANDLER(close,0)
SET_WIN_EVENT_HANDLER(minimize,0)
SET_WIN_EVENT_HANDLER(maximize,0)
SET_WIN_EVENT_HANDLER(rightclick,0)
SET_WIN_EVENT_HANDLER(filesdropped,1)
SET_WIN_EVENT_HANDLER(restore,0)

static value window_destroy( value w ) {
	val_check_kind(w,k_window);
	if( in_call )
		// Can't directly destroy while being in a callback,
		// 'post' destroy over message loop instead.
		system_sync_call((gen_callback)system_window_destroy,val_window(w));
	else
		system_window_destroy(val_window(w));
	return val_null;
}

static value window_set_title( value w, value t ) {
	val_check_kind(w,k_window);
	val_check(t, string);
	system_window_set_title(val_window(w),val_string(t));
	return val_null;
}

static value window_drag( value w ) {
	val_check_kind(w,k_window);
	system_window_drag(val_window(w));
	return val_null;
}

static value window_resize( value w, value o ) {
	val_check_kind(w,k_window);
	val_check(o,int);
	system_window_resize(val_window(w),val_int(o));
	return val_null;
}

static value window_set_prop( value w, value p, value b ) {
	val_check_kind(w,k_window);
	val_check(p,int);
	val_check(b,int);
	system_window_set_prop(val_window(w),val_int(p),val_int(b));
	return val_null;
}

static value window_get_prop( value w, value p ) {
	val_check_kind(w,k_window);
	val_check(p,int);
	return alloc_int(system_window_get_prop(val_window(w),val_int(p)));
}

static value window_get_handle( value w ) {
	val_check_kind(w,k_window);
	return alloc_abstract(k_window_handle,system_window_get_handle(val_window(w)));
}

static value window_add_message_hook( value w, value id1, value id2 ) {	
	val_check_kind(w,k_window);
	val_check(id1,int32);
	val_check(id2,int32);
	{
		msg_hook_list *n;
		msg_hook_list **ll;
		msg_hook_list *l = malloc(sizeof(struct _msg_hook_list));
		l->next = NULL;
		l->hook = malloc(sizeof(struct _window_msg_hook));
		memset(l->hook,0,sizeof(struct _window_msg_hook));
		l->hook->id1 = (void*) val_int32(id1);
		l->hook->id2 = (void*) val_int32(id2);
		ll = system_window_get_msg_hook_list(val_window(w));
		if (n=*ll) {			
			while (n->next) n=n->next;
			n->next = l;
		} else {
			*ll = l;						
		}			
		return alloc_abstract(k_window_msg_hook,l->hook);		
	}
}

static value window_remove_message_hook( value win, value hook ) {
	val_check_kind(win,k_window);
	val_check_kind(hook,k_window_msg_hook);
	{
		window *w = val_window(win);
		window_msg_hook *h = val_window_msg_hook(hook);
		msg_hook_list **ll = system_window_get_msg_hook_list(w);
		msg_hook_list *l = *ll;
		msg_hook_list *p = l;
		while (l) {
			if (l->hook == h) {
				if (p==l) {
					*ll = l->next;				
				} else {
					p->next = l->next;
				}
				free(l->hook);
				free(l);
				return alloc_int(1);
			}
			p=l;
			l=l->next;
		}
		return alloc_int(0);
	}
}

void* window_invoke_msg_hooks( window *w, void *id1, void *id2, void *p1, void *p2 ) {
	msg_hook_list *l = *system_window_get_msg_hook_list(w);
	while(l) {
		if (l->hook->id1 == id1 && l->hook->id2 == id2) {
			l->hook->p1 = p1;
			l->hook->p2 = p2;
			if (l->hook->fc) {
				// direct C to C invokation
				void *result = l->hook->fc(l->hook,id1,id2,p1,p2); 
				if (result)
					return result;
			} else if (l->hook->fn) {
				// Neko invokation
				value exc = NULL;
				value result = val_callEx(val_null,l->hook->fn,NULL,0,&exc);
				if( exc != NULL )
					val_rethrow(exc);
				if( val_int(result) )
					return (void*)val_int(result);
			}
		}
		l = l->next;		
	}
	return 0;
}

/* ************************************************************************* */
// MESSAGE HOOK


static value msghook_set_c_callback( value h, value f ) {
	val_check_kind(h,k_window_msg_hook);
	val_check_kind(f,k_window_msg_cb);
	{
		window_msg_hook *hook = val_window_msg_hook(h);
		hook->fc = val_window_msg_cb(f);
		return val_null;
	}	
}

static value msghook_set_n_callback( value h, value f ) {
	val_check_kind(h,k_window_msg_hook);
	// TO DO: add proper function type checking
	{
		window_msg_hook *hook = val_window_msg_hook(h);
		hook->fn = f;
		return val_null;
	}
}

static value msghook_get_param1( value h ) {
	val_check_kind(h,k_window_msg_hook);
	{
		window_msg_hook *hook = val_window_msg_hook(h);
		return alloc_int32(hook->p1);
	}
}

static value msghook_get_param2( value h ) {
	val_check_kind(h,k_window_msg_hook);
	{
		window_msg_hook *hook = val_window_msg_hook(h);
		return alloc_int32(hook->p2);
	}
}

/* ************************************************************************* */
// FLASH

static const char *flash_on_call_event( flash *f, const char *ident, const char *params, int *size ) {
	value *root = (value*)flashp_get_private(f);
	value args[2];
	value exc = NULL,r;
	if( root == NULL )
		return NULL;
	args[0] = alloc_string(ident);
	args[1] = alloc_string(params);
	in_call++;
	r = val_callEx(val_null,*root,args,2,&exc);
	in_call--;
	if( exc != NULL )
		val_rethrow(exc);
	if( !val_is_string(r) )
		return NULL;
	if( size != 0 )
		*size = val_strlen(r);
	return val_string(r);
}

static value flash_new( value wv ) {
	window *w;
	private_data *p;
	val_check_kind(wv,k_window);
	w = val_window(wv);
	p = system_window_get_private(w);
	if( p->player != NULL )
		neko_error();
	p->player = flashp_new(w);
	if( p->player == NULL )
		neko_error();
	p->vplayer = alloc_abstract(k_flash,p->player);
	flashp_set_call(p->player,flash_on_call_event);
	return p->vplayer;
}

static value flash_set_attribute( value f, value att, value val ) {
	val_check_kind(f,k_flash);
	val_check(att,string);
	val_check(val,string);
	flashp_set_attribute(val_flash(f),val_string(att),val_string(val));
	return val_null;
}

static value flash_get_attribute( value f, value att ) {
	const char *v;
	val_check_kind(f,k_flash);
	val_check(att,string);
	v = flashp_get_attribute(val_flash(f),val_string(att));
	if( v == NULL )
		return val_null;
	return alloc_string(v);
}

static value flash_start( value f ) {
	val_check_kind(f,k_flash);
	if( !flashp_start(val_flash(f)) )
		neko_error();
	return val_null;
}

static value flash_destroy( value vf ) {
	flash *f;
	private_data *p;
	value *root;
	val_check_kind(vf,k_flash);
	if( in_call )
		val_throw(alloc_string("Cannot destroy flash in event callback"));
	f = val_flash(vf);
	p = system_window_get_private(flashp_get_window(f));
	root = flashp_get_private(f);
	p->player = NULL;
	p->vplayer = NULL;
	val_kind(f) = NULL;
	flashp_destroy(f);
	if( root != NULL )
		free_root(root);
	return val_null;
}

static value flash_on_call( value vf, value callb, value onurl ) {
	value *root;
	flash *f;
	val_check_kind(vf,k_flash);
	val_check_function(callb,2);
	val_check_function(onurl,3);
	f = val_flash(vf);
	root = (value*)flashp_get_private(f);
	if( root == NULL ) {
		root = alloc_root(2);
		flashp_set_private(f,root);
	}
	root[0] = callb;
	root[1] = onurl;
	return val_null;
}

static value flash_call( value f, value ident, value params ) {
	char *r;
	val_check_kind(f,k_flash);
	val_check(ident,string);
	val_check(params,string);
	r = flashp_call_in(val_flash(f),val_string(ident),val_strlen(ident),val_string(params),val_strlen(params));
	if( r == NULL )
		neko_error();
	{
		value v = alloc_string(r);
		free(r);
		return v;
	}
}

/* ************************************************************************* */
// STREAM

typedef struct {
	NPP inst;
	NPStream stream;
	uint16 stype;
	int pos;
	int notify;
	flash *f;
} stream;

typedef struct {
	stream *s;
	char *buf;
	int size;
} stream_buffer;

static void do_free_stream( stream *s ) {
	// if the app has not yet been closed
	if( fl_dll != NULL ) {
		if( s->notify != -1 )
			fl_dll->table.urlnotify(s->inst,s->stream.url,s->notify?NPRES_DONE:NPRES_NETWORK_ERR,s->stream.notifyData);
		fl_dll->table.destroystream(s->inst,&s->stream,NPRES_DONE);
	}
	free((char*)s->stream.url);
	free_stream(s);
}

static void do_free_stream_sync( void *_r ) {
	stream *s = *(stream**)_r;
	free_root((value*)_r);
	do_free_stream(s);
}

// called by NP_HOST
void flashp_url_process( flash *f, const char *url, const char *postData, int postLen, void *notifyData ) {
	stream *s = alloc_stream();
	int success;
	value *root;
	memset( &s->stream, 0, sizeof( NPStream ) );
	s->inst = flashp_get_npp(f);
	s->stream.notifyData = notifyData;
	s->stream.url = strdup(url);
	s->pos = 0;
	s->notify = -1;
	success = (fl_dll->table.newstream(s->inst,"application/x-shockwave-flash",&s->stream,0,&s->stype) == NPERR_NO_ERROR);
	root = (value*)flashp_get_private(f);
	if( root == NULL )
		success = 0;
	if( !success ) {
		fl_dll->table.urlnotify(s->inst,url,NPRES_NETWORK_ERR,notifyData);
		free_stream(s);
		return;
	}
	{
		value vs = alloc_abstract(k_stream,s);
		value args[3] = {
			vs,
			alloc_string(url),
			postData?copy_string(postData,postLen):val_null,
		};
		val_callN(root[1],args,3);
	}
}

static value stream_size( value vs, value size ) {
	stream *s;
	val_check_kind(vs,k_stream);
	val_check(size,int);
	s = val_stream(vs);
	s->stream.end = val_int(size);
	return val_null;
}

static value stream_close( value vs, value success ) {
	stream *s;
	val_check_kind(vs,k_stream);
	val_check(success,bool);
	s = val_stream(vs);
	s->notify = val_bool(success);
#	ifdef STREAM_SYNC
	if( !system_is_main_thread() ) {
		value *r = alloc_root(1);
		*r = (value)s;
		system_sync_call(do_free_stream_sync,r);
	} else
		do_free_stream(s);
#	else
	do_free_stream(s);
#	endif
	val_kind(vs) = NULL;
	return val_null;
}

#ifdef STREAM_SYNC

static int stream_data( stream *s, char *buf, int size );

static void stream_data_sync( void *_sb ) {
	stream_buffer *sb = (stream_buffer*)_sb;
	int pos = 0;
	int len = sb->size;
	while( len > 0 ) {
		int bytes = stream_data(sb->s, sb->buf + pos, len);
		pos += bytes;
		len -= bytes;
	}
	free(sb->buf);
	free(sb);
}

#endif

static int stream_data( stream *s, char *buf, int size ) {
	int len;	
#	ifdef STREAM_SYNC
	if (!system_is_main_thread()) {
		stream_buffer *sb = (stream_buffer*)malloc(sizeof(stream_buffer));
		sb->s = s;
		sb->buf = (char*)malloc(size);
		sb->size = size;
		memcpy(sb->buf,buf,size);
		system_sync_call(stream_data_sync,sb);
		return size; // we will make sure that all the buffer is written, see over
	}	
#	endif
	len = fl_dll->table.writeready(s->inst,&s->stream);
	if( len <= 0 )
		return len;
	if( len > size )
		len = size;
	len = fl_dll->table.write(s->inst,&s->stream,s->pos,len,buf);
	s->pos += len;
	return len;
}

static value stream_char( value vs, value c ) {
	char cc;
	val_check_kind(vs,k_stream);
	val_check(c,int);
	cc = val_int(c) & 0xFF;
	if( stream_data(val_stream(vs),&cc,1) != 1 )
		neko_error();
	return val_null;
}

static value stream_bytes( value vs, value buf, value pos, value len ) {
	val_check_kind(vs,k_stream);
	val_check(buf,string);
	val_check(pos,int);
	val_check(len,int);
	if( val_int(pos) < 0 || val_int(len) < 0 || val_int(pos) + val_int(len) > val_strlen(buf) )
		neko_error();
	return alloc_int(stream_data(val_stream(vs),val_string(buf)+val_int(pos),val_int(len)));
}

/* ************************************************************************* */
// EXPORTS

DEFINE_PRIM(initialize,1);
DEFINE_PRIM(loop,0);
DEFINE_PRIM(loop_exit,0);
DEFINE_PRIM(cleanup,0);
DEFINE_PRIM(plugin_file_version,1);
DEFINE_PRIM(sync_call,1);
DEFINE_PRIM(is_main_thread,0);

DEFINE_PRIM(window_create,4);
DEFINE_PRIM(window_show,2);
DEFINE_PRIM(window_destroy,1);
DEFINE_PRIM(window_set_title,2);
DEFINE_PRIM(window_drag,1);
DEFINE_PRIM(window_resize,2);
DEFINE_PRIM(window_set_prop,3);
DEFINE_PRIM(window_get_prop,2);
DEFINE_PRIM(window_get_handle,1);
DEFINE_PRIM(window_add_message_hook,3);
DEFINE_PRIM(window_remove_message_hook,2);

DEFINE_PRIM(msghook_set_c_callback,2);
DEFINE_PRIM(msghook_set_n_callback,2);
DEFINE_PRIM(msghook_get_param1,1);
DEFINE_PRIM(msghook_get_param2,1);

DEFINE_PRIM(flash_new,1);
DEFINE_PRIM(flash_set_attribute,3);
DEFINE_PRIM(flash_get_attribute,2);
DEFINE_PRIM(flash_start,1);
DEFINE_PRIM(flash_call,3);
DEFINE_PRIM(flash_on_call,3);
DEFINE_PRIM(flash_destroy,1);

DEFINE_PRIM(stream_size,2);
DEFINE_PRIM(stream_close,2);
DEFINE_PRIM(stream_char,2);
DEFINE_PRIM(stream_bytes,4);

