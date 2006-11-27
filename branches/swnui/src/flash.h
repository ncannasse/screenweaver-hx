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
#ifndef FLASH_H
#define FLASH_H

#include "system.h"

#define	SPECIAL_IDENTIFIER  0x0FEEBBCC
#define SPECIAL_METHOD_NAME "swhxCall"

#ifdef OSX
#		define strcmpi strcasecmp
#endif

typedef struct _flash flash;
typedef const char *(*on_call)( flash *f, const char *s, const char *params, int *size );

flash *flashp_new( window *w );
void flashp_set_attribute( flash *f, const char *attname, const char *value );
const char *flashp_get_attribute( flash *f, const char *attname );
void flashp_set_iattribute( flash *f, const char *attname, int value );
int flashp_start( flash *f );
void flashp_destroy( flash *f );

window *flashp_get_window( flash *f );
NPP flashp_get_npp( flash *f );

void flashp_set_call( flash *f, on_call callb );
const char *flashp_call( flash *f, const char *s, const char *params, int *size );
void flashp_set_private( flash *f, void *priv );
void *flashp_get_private( flash *f );

char *flashp_call_in( flash *f, const char *s, const int ssize, const char *params, const int psize );
const char *flashp_get_location( flash *f );

void flashp_url_process( flash *f, const char *url, const char *postData, int postLen, void *notifyData );

#endif

