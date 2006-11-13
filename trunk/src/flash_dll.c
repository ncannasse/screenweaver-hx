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
#include "flash_dll.h"

extern void getHostTable(NPNetscapeFuncs *np_host_table);

flash_dll *dll_init( const char *path, const char **error ) {
	library *l = system_library_open(path);
	flash_dll *dll;
	NP_DISABLELOCSEC dislocsec;
	NP_GETENTRYPOINTS getentpts;
	void *init;	
	NP_PLUGINSHUTDOWN sd;
	NP_SETUP su;	
	if( l == NULL ) {
		*error = "Unable to load Flash player library";
		return NULL;
	}
	dislocsec = (NP_DISABLELOCSEC)system_library_symbol(l,SYM_NP_DISABLELOCSEC);
	getentpts = (NP_GETENTRYPOINTS)system_library_symbol(l,SYM_NP_GETENTRYPOINTS);
	init = (NP_PLUGININIT_V2)system_library_symbol(l,SYM_NP_PLUGININIT);
	sd = (NP_PLUGINSHUTDOWN)system_library_symbol(l,SYM_NP_PLUGINSHUTDOWN);
	su = (NP_SETUP)system_library_symbol(l,SYM_NP_FLASHSETUP);	
	if( init == NULL || sd == NULL ) {
		*error = "Unable to retrieve Flash player init symbols";
		return NULL;
	}

	// Undocumented 'FlashPlugin_Setup' method.
	// No idea about its functioning, so skipping it for now:
	// if (su)
	//	err = su();

	dll = (flash_dll*)malloc(sizeof(struct _flash_dll));
	dll->lib = l;
	dll->shutdown = sd;
	dll->table.size = sizeof(NPPluginFuncs);
	dll->disable_loc_sec = dislocsec;		
	getHostTable(&dll->host_table);
	
	if(getentpts) {
		if( getentpts(&dll->table) != NPERR_NO_ERROR ) {
			*error = "Unable to retrieve Flash player function table";
			free(dll);
			return NULL;
		}
		if( ((NP_PLUGININIT_V1)init)(&dll->host_table) != NPERR_NO_ERROR ) {
			*error = "Unable to initialize Flash player (v1)";
			free(dll);
			return NULL;
		}
	} else {
		if( ((NP_PLUGININIT_V2)init)(&dll->host_table,&dll->table) != NPERR_NO_ERROR ) {
			*error = "Unable to initialize Flash player (v2)";
			free(dll);
			return NULL;
		}
	}
	
	// see docs at: http://www.adobe.com/devnet/flash/articles/fplayer8_security_08.html
	if( dislocsec && dislocsec() != NPERR_NO_ERROR ) {
		*error = "Unable to disable local security on Flash player";
		free(dll);
		return NULL;
	}
	return dll;
}


void dll_close( flash_dll *dll ) {
	((NP_PLUGINSHUTDOWN)dll->shutdown)();
	system_library_close(dll->lib);
	free(dll);
}

