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
#ifndef FLASH_DLL
#define FLASH_DLL

#ifdef _WIN32
typedef int bool;
#endif
#include <npapi/npapi.h>
#include <npapi/npupp.h>
#include "system.h"

#ifdef LINUX
#	include <stdlib.h> 
#	include <string.h>
#endif

#ifdef _WIN32
#	define NS_4XPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#else
#	define NS_4XPLUGIN_CALLBACK(_type, _name) _type (* _name)
#endif

typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGININIT_V2)  (const NPNetscapeFuncs* pnCallbacks, NPPluginFuncs* ppCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGININIT_V1)  (const NPNetscapeFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_DISABLELOCSEC)  (void);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_SETUP)			 (void);

#define SYM_NP_PLUGININIT		"NP_Initialize"
#define SYM_NP_GETENTRYPOINTS	"NP_GetEntryPoints"
#define SYM_NP_PLUGINSHUTDOWN	"NP_Shutdown"
#define SYM_NP_DISABLELOCSEC	"Flash_DisableLocalSecurity"
#define SYM_NP_FLASHSETUP		"FlashPlugin_Setup"

typedef struct _flash_dll {
	NPPluginFuncs table;
	NPNetscapeFuncs host_table;
	library *lib;
	void *shutdown;
	NP_DISABLELOCSEC disable_loc_sec;
} flash_dll;

flash_dll *dll_init( const char *path, const char **error );
void dll_close( flash_dll *dll );

#endif
