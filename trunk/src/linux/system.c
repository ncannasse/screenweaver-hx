#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <pthread.h>

#include "npapi/npapi.h"
#include "npapi/npupp.h"
#include "xembed.h"
#include "gtk2xtbin.h"

#include "../system.h"
#include "../flash_dll.h"
#include "../flash.h"
#include <neko/neko.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

window_list *windows = NULL;
pthread_t main_thread_id;

extern flash_dll *fl_dll;

int system_init() {
	XInitThreads();
	gtk_init(NULL,NULL);
	main_thread_id = pthread_self();
    return 0;
}

void system_cleanup() {
	
}

void system_loop() {
	gtk_main();
}

void system_loop_exit() {
	gtk_main_quit();
}

library *system_library_open( const char *path ) {
	void *result = NULL;
	char *expanded;
	if (path && path[0]=='~') {
		expanded = malloc(PATH_MAX+1);
		char *home = getenv("HOME");
		strcpy(expanded,home);
		strncat(expanded,path+1,PATH_MAX-strlen(home));
	} else {
		expanded = strdup(path);
	}
	#ifdef DEBUG
	fprintf(stderr,"system_library open: %s\n",expanded);
	#endif
	result = (void*) dlopen(expanded,RTLD_LAZY);
	free(expanded);
	return result;
}

void *system_library_symbol( library *l, const char *symbol ) {
	return (library*)dlsym( (void*)l, symbol );
}

void system_library_close( library *l ) {
	dlclose( (void*)l );
}

char *system_fullpath( const char *file ) {
	char rp[PATH_MAX+1];
	#ifdef DEBUG
	fprintf(stderr,"system_fullpath called: %s\n", file);
	#endif
	if(realpath(file,rp)) {
		char *result = malloc(PATH_MAX+8);
		strcpy(result,"file://");
		strncat(result,rp,PATH_MAX);
		#ifdef DEBUG
		fprintf(stderr,"returning: '%s'\n",result);		
		#endif
		return result;
	}	
	return strdup(file);	
}

char *system_plugin_file_version( const char *file ){
	fprintf(stderr,"system_plugin_version (TODO!) (%s)\n",file);
	// TODO: get .SO file version, if .SOs carry version info?
	// ignore 'flashplayer.bin' for now:
	if (memcmp("flashplayer.bin",file,strlen(file))==0)		 
		return NULL;
	// and return version 9 for anything else that exists:
	else {
		struct stat buf;
		if( stat( file, &buf ) == -1 )
			return NULL;
	}
	return strdup("9.0");
}

struct _window {
	GtkWidget *gwindow;
	NPWindow npwin;
	void *flash;	
	on_event evt;
	on_npevent npevent;
	private_data *p;
	
	enum WindowFlags flags;
};

window* system_window_create( const char *title, int width, int height,enum WindowFlags flags, on_event f){		
	
	window*  w = malloc(sizeof(struct _window));
	w->evt = f;	
	w->flags = flags ^ WF_FLASH_RUNNING;
	w->gwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	w->npevent == NULL;
	w->p = NULL;
	
	gtk_window_set_default_size(GTK_WINDOW(w->gwindow), width,height);
	gtk_widget_show(w->gwindow);
	
	GdkWindow *gdkwin = w->gwindow->window;
    GtkWidget *xt_bin = gtk_xtbin_new(gdkwin, NULL);
    gtk_widget_show(xt_bin);
	NPWindow *npwin  = &(w->npwin);
	npwin->window = (void *)GTK_XTBIN (xt_bin)->xtwindow;
    npwin->x = 0;
    npwin->y = 0;
    npwin->width = width;
    npwin->height = height;
    npwin->type = NPWindowTypeWindow;
    npwin->clipRect.top       = 0; 
    npwin->clipRect.left      = 0;
    npwin->clipRect.right     = width;
    npwin->clipRect.bottom    = height;
	
	NPSetWindowCallbackStruct* npws=calloc(sizeof(NPSetWindowCallbackStruct),1);
    npws->type = NP_SETWINDOW;
    npws->depth = gdk_window_get_visual(gdkwin)->depth;
    npws->display = GTK_XTBIN(xt_bin)->xtdisplay;
    npws->visual = GDK_VISUAL_XVISUAL(gdk_window_get_visual(gdkwin));
    npws->colormap = GDK_COLORMAP_XCOLORMAP(gdk_window_get_colormap(gdkwin));
    npwin->ws_info = npws;
	
	gtk_xtbin_resize (xt_bin, width, height);
    gdk_flush();
	
	return w;
}

void system_window_show( window *w, int show ){

}


void system_window_set_npevent( window *w, on_npevent f ) {
	w->npevent = f;
}

void system_window_set_private( window *w, private_data *p ) {
	w->p = p;
}

void system_window_destroy( window *w ){
	
}

void system_window_destroy_take_void(void *w){

}


private_data *system_window_get_private( window *w ) {
	return w->p;
}

void *system_window_get_handle( window *w ) {
	return w->gwindow;
}

NPWindow* system_window_getnp( window *w ) {
	return &w->npwin;
}

void system_window_invalidate( window *w, NPRect *r ){
	#ifdef DEBUG
		fprintf(stderr,"system_window_invalidate NYI\n");
	#endif
}

void system_window_set_flash( window *w, void *f ) {
	w->flash = f;
}

void system_window_set_title( window *w, const char* title ){
	
}

void system_window_drag( window *w ){
	
}

void system_window_resize( window *w, int o ){
	
}


void system_window_set_prop( window *w, enum WindowProperty prop, int value ){
	
}

int system_window_get_prop( window *w, enum WindowProperty prop ){
	return 0;
}

typedef struct {
	void (*func)( void * );
	void *param;
} sync_call_data;

static gint gtk_call_synced( gpointer _data ) {
	sync_call_data* data = (sync_call_data*)_data;
	#ifdef DEBUG
	fprintf(stderr,"now calling synced: %p %p\n", data->func, data->param );
	#endif
	data->func( data->param );
	free( data );
	return FALSE;
}

void system_sync_call( void func( void * ), void *param ){
	sync_call_data* data = (sync_call_data*)malloc( sizeof(sync_call_data) );
	data->func = func;
	data->param = param;
	#ifdef DEBUG
	fprintf(stderr,"scheduling synced call: %p %p\n", data->func, data->param );
	#endif
	gtk_idle_add( gtk_call_synced, (gpointer)data );
}

int system_is_main_thread(){
	#ifdef DEBUG
	fprintf(stderr,"system_is_main_thread (%s)\n", pthread_equal(main_thread_id,pthread_self())?"yes":"no" );
	#endif
	return pthread_equal(main_thread_id,pthread_self());
}

void system_launch_url( const char *url ) {
	#ifdef DEBUG
	fprintf(stderr,"should launch URL: %s\n", url );
	#endif
}
