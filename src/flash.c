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
#include <stdio.h>
#include "flash_dll.h"
#include "flash.h"

#ifdef LINUX
#	include <stdlib.h> 
#	include <string.h>
#	define strcmpi strcasecmp
#endif

#define FLASH_MIME		"application/x-shockwave-flash"

struct attributes {
	int count;
	char **names;
	char **values;
};

struct _flash {
	window *wnd;
	struct attributes att;
	int started;
	struct _NPP id;
	on_call call;
	void *priv;
	NPObject *object;
	char *location;
};

extern flash_dll *fl_dll;
extern int utf8length( const char *s );
extern void DoReleaseObject( NPObject *npobj );
extern void DoReleaseVariant( NPVariant *variant );

flash *flashp_new( window *w ) {
	char *wmode = system_window_get_prop(w,WP_TRANSPARENT) ? "transparent" : "opaque";
	flash *f = (flash*)malloc(sizeof(struct _flash));
	system_window_set_flash(w,(void*)f);
	f->wnd = w;
	f->att.count = 0;
	f->att.names = NULL;
	f->att.values = NULL;
	f->started = 0;
	f->call = NULL;
	f->priv = NULL;
	f->object = NULL;
	f->location = NULL;

	// internal id
	f->id.ndata = f;
	f->id.pdata = 0;

	// set default values
	flashp_set_attribute( f, "type", FLASH_MIME );
	flashp_set_attribute( f, "allowscriptaccess", "always" );
	flashp_set_attribute( f, "quality", "high" );
	flashp_set_attribute( f, "wmode", wmode );
	flashp_set_attribute( f, "salign", "LT" );
	flashp_set_attribute( f, "scale", "exactfit" );

	return f;
}

window *flashp_get_window( flash *f ) {
	return f->wnd;
}

NPP flashp_get_npp( flash *f ) {
	return &f->id;
}

void flashp_set_call( flash *f, on_call callb ) {
	f->call = callb;
}

void flashp_set_private( flash *f, void *priv ) {
	f->priv = priv;
}

void *flashp_get_private( flash *f ) {
	return f->priv;
}

const char *flashp_call( flash *f, const char *s, const char *params, int *size ) {
	if( f->call == NULL )
		return NULL;
	return f->call(f,s,params,size);
}

const char *flashp_get_attribute( flash *f, const char *attname ) {
	int i;
	for(i=0;i<f->att.count;i++) {
		if( strcmpi(f->att.names[i],attname) == 0 )
			return f->att.values[i];
	}
	return NULL;
}

void flashp_set_attribute( flash *f, const char *attname, const char *value ) {
	int i;
	for(i=0;i<f->att.count;i++) {
		if( strcmpi(f->att.names[i],attname) == 0 ) {
			free(f->att.values[i]);
			f->att.values[i] = strdup(value);
			break;
		}
	}
	if( i == f->att.count ) {
		f->att.count++;
		f->att.names = (char**)realloc(f->att.names,sizeof(char*) * (i + 1));
		f->att.values = (char**)realloc(f->att.values,sizeof(char*) * (i + 1));
		f->att.names[i] = strdup(attname);
		f->att.values[i] = strdup(value);
	}
}

void flashp_set_iattribute( flash *f, const char *attname, int value ) {
	char buf[24];
	sprintf(buf,"%d",value);
	flashp_set_attribute(f,attname,buf);
}

void flashp_destroy( flash *f ) {
	int i;
	if (f->wnd) system_window_set_prop(f->wnd,WP_FLASH_RUNNING,0);
	if( f->object != NULL )
		DoReleaseObject(f->object);
	if( f->started )
		fl_dll->table.destroy(&f->id,0);
	for(i=0;i<f->att.count;i++) {
		free(f->att.names[i]);
		free(f->att.values[i]);
	}
	free(f->att.names);
	free(f->att.values);
	free(f->location);
	free(f);
}

int flashp_start( flash *f ) {
	NPError e;
	const char *src = flashp_get_attribute(f,"SRC");

	if( f->started )
		return 0;

	// set the location property
	if( src != NULL ) {
		char *src_unc = strdup(src);
		int i = (int)strlen(src_unc);
		while(i) {
			if( src_unc[i]=='\\' ) src_unc[i] = '/';
			i--;
		}
		f->location = system_fullpath(src_unc);
		free(src_unc);
	} else
		f->location = strdup("empty");

	e = fl_dll->table.newp(FLASH_MIME, &f->id, NP_EMBED, f->att.count, f->att.names, f->att.values, 0);
	if( e != NPERR_NO_ERROR )
		return 0;
	f->started = 1;

	e = fl_dll->table.getvalue(&f->id,NPPVpluginScriptableNPObject,&f->object);
	if( e != NPERR_NO_ERROR )
		f->object = NULL;

	fl_dll->table.setwindow(&f->id,system_window_getnp(f->wnd));

	// if src is defined, then need to start the stream
	if( src != NULL )
		flashp_url_process(f,src,NULL,0,NULL);

	if (f->wnd) system_window_set_prop(f->wnd,WP_FLASH_RUNNING,1);
	return 1;
}


char *flashp_call_in( flash *f, const char *s, const int ssize, const char *params, const int psize ) {
	NPVariant p[2], r;
	char *result;
	if( f->object == NULL )
		return NULL;
	p[0].type = NPVariantType_String;
	p[0].value.stringValue.utf8characters = s;
	p[0].value.stringValue.utf8length = ssize;
	p[1].type = NPVariantType_String;
	p[1].value.stringValue.utf8characters = params;
	p[1].value.stringValue.utf8length = psize;
	r.type = NPVariantType_Void;
	if( !f->object->_class->invoke( f->object, (NPIdentifier)SPECIAL_IDENTIFIER, (NPVariant *)p, 2, &r ) )
		return NULL;
	if( r.type == NPVariantType_String )
		result = strdup(r.value.stringValue.utf8characters);
	DoReleaseVariant(&r);
	return result;
}

const char *flashp_get_location( flash *f ) {
	return f->location;
}

