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
#include "system.h"

#ifdef LINUX
#	include <stdlib.h> 
#	include <stdarg.h>
#	include <string.h>
#endif

#include <stddef.h>
#ifndef _MSC_VER
#	include <stdint.h>
#endif

typedef intptr_t int_val;

// Note that different agent strings, cause different plug-in behavior
#ifdef WIN32
#	define AGENT "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.0.3) Gecko/20060426 Screenweaver HX/1.0.0.0"
#elif OSX
#	define AGENT "Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en) AppleWebKit/418 (KHTML, like Gecko) Safari/417.9.3"
#else
#	define AGENT "???"
#endif

#ifdef WIN32
	/*static*/ void DO_TRACE( const char* STR, ... ) {
		static char msg[1024*64];
		va_list vl;
		va_start(vl, STR);
		sprintf( msg, "[%i] ", GetCurrentThreadId());
		vsprintf( msg+strlen(msg), STR, vl );
		va_end(vl);
		strcat( msg, "\n" );
		OutputDebugString( msg );
	};
	static void NO_TRACE( const char* STR, ... ) {
	}
#else
#	define DO_TRACE( STR, ...) fprintf( stderr, STR "\n", ## __VA_ARGS__ )
#	define NO_TRACE( STR, ...)
#endif

#ifdef _DEBUG
#	define TRACE			DO_TRACE
#	define SCRIPT_TRACE		DO_TRACE
#else
#	define TRACE			NO_TRACE
#	define SCRIPT_TRACE		NO_TRACE
#endif

#undef NO_ERROR
#define NO_ERROR			NPERR_NO_ERROR
#define NO_IDENTIFIER		((NPIdentifier)0)

#define FLASH_REQUEST		"__flash__request"
#define FSCMD				"_DoFSCommand"

extern flash_dll *fl_dll;

static NPObject* CreateObject( NPP npp, NPClass *aClass );
static NPObject *RetainObject( NPObject *npobj );

static bool Invoke( NPP npp, NPObject *npobj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result ) ;
static bool InvokeDefault( NPP npp, NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result ) ;
static bool GetProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName, NPVariant *result ) ;
static bool SetProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName, const NPVariant *value ) ;
static bool RemoveProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName ) ;
static bool HasProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName ) ;
static bool HasMethod( NPP npp, NPObject *npobj, NPIdentifier methodName ) ;

// --- URL

static void url_process( NPP instance, const char *url, const char *post, int postLen, void *notifyData ) {
	char *url2 = (char*)url;
	if( memcmp(url,"http://",7) != 0 && memcmp(url,"https://",8) != 0 && memcmp(url,"file://",7) != 0 ) {
		// remove trailing '?' for flash9
		int l;
		url2 = system_fullpath(url);
		l = (int)strlen(url2)-1;
		if( l > 0 && url2[l] == '?' )
			url2[l] = 0;
	}
	flashp_url_process((flash*)instance->ndata,url2,post,postLen,notifyData);
	if( url2 != url )
		free(url2);
}

// --- Property IDs

static char **np_ids = NULL;
static int_val np_id_count = 0;

static NPIdentifier resolveNPId( const char *id ) {
	int_val i;
	for(i=0;i<np_id_count;i++)
		if( strcmp(np_ids[i],id) == 0 )
			return (NPIdentifier)(i+1);
	if( strcmp(id,SPECIAL_METHOD_NAME) == 0 )
		return (NPIdentifier)SPECIAL_IDENTIFIER;
	return NO_IDENTIFIER;
}

static NPIdentifier addNPId( const char *id ) {
	NPIdentifier newid = resolveNPId(id);
	if( newid == NO_IDENTIFIER ) {
		np_id_count++;
		SCRIPT_TRACE("New npid added: %i == %s",np_id_count,id);
		np_ids = realloc(np_ids,np_id_count*sizeof(char*));
		np_ids[np_id_count-1] = strdup(id);
		return (NPIdentifier)np_id_count;
	}
	return newid;
}

static const char *getNPId( NPIdentifier id ) {
	int_val index = ((int_val)id)-1;
	if( index >= 0 && index < np_id_count )
		return np_ids[index];
	if( id == (NPIdentifier)SPECIAL_IDENTIFIER )
		return SPECIAL_METHOD_NAME;
	return NULL;
}

static int matchNPId(NPIdentifier id, const char *str) {
	const char *strid = getNPId(id);
	return ( strid != NULL && strcmp(strid,str) == 0 );
}

void freeNPIds() {
	while( np_id_count )
		free(np_ids[--np_id_count]);
	free(np_ids);
}

// --- Simulated Browser Objects

// Window class;
static NPClass __gen_class =
	{ NP_CLASS_STRUCT_VERSION
	, (NPAllocateFunctionPtr) malloc
	, (NPDeallocateFunctionPtr) free
	, 0
	, (NPHasMethodFunctionPtr) HasMethod
	, (NPInvokeFunctionPtr) Invoke
	, (NPInvokeDefaultFunctionPtr)InvokeDefault
	, (NPHasPropertyFunctionPtr) HasProperty
	, (NPGetPropertyFunctionPtr) GetProperty
	, (NPSetPropertyFunctionPtr) SetProperty
	, (NPRemovePropertyFunctionPtr) RemoveProperty
	};
static NPObject __window = { &__gen_class, 1 };
static NPObject __location = { &__gen_class, 1};
static NPObject __top = { &__gen_class, 1 };
static NPObject __top_location = { &__gen_class, 1 };

static void traceObjectOnCall(const char *f, NPObject *o){
	if (o == &__top) TRACE("DOM object 'top': %s",f);
	else if (o == &__window) TRACE("DOM object 'window': %s",f);
	else if (o == &__location) TRACE("DOM object 'location': %s",f);
	else if (o == &__top_location) TRACE("DOM object 'top.location': %s",f);
}

static void Status_( NPP instance, const char* message ) {
	TRACE( "Status" );
}

static const char *UserAgent( NPP instance ) {
	TRACE( "UserAgent %s", AGENT );
	return AGENT;
}

static uint32 MemFlush( uint32 size ) {
	TRACE( "MemFlush %i", size );
	return 0;
}

static void ReloadPlugins( NPBool reloadPages ) {
	TRACE( "ReloadPlugins %s" , reloadPages?"TRUE":"0" );
}

static JRIEnv *GetJavaEnv(void) {
	TRACE( "GetJavaEnv" );
	return NULL;
}

static void *GetJavaPeer( NPP instance ) {
	TRACE( "GetJavaPeer" );
	return NULL;
}

static NPError GetValue( NPP instance, NPNVariable variable, void *value ) {
	switch (variable) {
#ifdef _WIN32
		case NPNVnetscapeWindow: {
			flash *f = (flash*)instance->ndata;
			*(HWND*)value = (HWND)system_window_get_handle(flashp_get_window(f));
			return NPERR_NO_ERROR;
		}
#endif
		case NPNVWindowNPObject: {
			TRACE ("WindowNPObject requested");
			*(NPObject**)value = &__window;
			RetainObject(&__window);
			return NPERR_NO_ERROR;
		}
	}
	TRACE( "GetValue %i", variable );
	return NO_ERROR;
}

static NPError SetValue( NPP instance, NPPVariable variable, void *value ) {
	switch(variable) {
		case NPPVpluginWindowBool:
			TRACE( "NPPVpluginWindowBool - %i", value);
			break;
		default:
			TRACE( "SetValue %i", variable );
			break;
	}
	return NO_ERROR;
}

static NPError GetURLNotify( NPP instance, const char* url, const char* target, void* notifyData ) {
	TRACE( "GetURLNotify (url) %s (target) %s (nd) %i", url, target, notifyData );

	if (target && strlen(target)==6 && memcmp("_blank",target,6)==0) {
		system_launch_url(url);
		fl_dll->table.urlnotify(instance,url,NPRES_DONE,notifyData);
		return NO_ERROR;
	}

	if( memcmp(url,"javascript:",11) == 0 ) {
		NPStream s;
		uint16 stype;
		int success;
		memset(&s,0,sizeof(NPStream));
		s.url = strdup(url);
		success = (fl_dll->table.newstream(instance,"text/html",&s,0,&stype) == NPERR_NO_ERROR);		
		if( success ) {
			int pos = 0;
			int size;
			char buf[256];
			sprintf(buf,"%X__flashplugin_unique__",(int_val)instance);
			size = (int)strlen(buf);
			s.end = size;
			while( pos < size ) {
				int len = fl_dll->table.writeready(instance,&s);
				if( len <= 0 )
					break;
				if( len > size - pos )
					len = size - pos;
				len = fl_dll->table.write(instance,&s,pos,len,buf+pos);
				if( len <= 0 )
					break;
				pos += len;
			}
			success = (pos == size);
		}
		fl_dll->table.urlnotify(instance,url,success?NPRES_DONE:NPRES_NETWORK_ERR,notifyData);
		fl_dll->table.destroystream(instance,&s,NPRES_DONE);
		free((void*)s.url);
	} else
		url_process(instance,url,NULL,0,notifyData);
	return NO_ERROR;
}

static NPError GetURL( NPP instance, const char* url, const char* target ) {
	TRACE( "GetURL %s", url );
	if (target && strlen(target)==6 && memcmp("_blank",target,6)==0) {
		system_launch_url(url);
		return NO_ERROR;
	}
	return NO_ERROR;
}

static NPError PostURLNotify( NPP instance, const char* url, const char* target, uint32 len, const char* buf, NPBool file, void* notifyData) {
	TRACE( "PostURLNotify (url) %s (target) %s (buf) %s[%d] (file) %i (nd) %i", url, target,  buf, len, file, notifyData);
	url_process(instance,url,buf,len,notifyData);
	return NO_ERROR;
}

static NPError PostURL( NPP instance, const char* url, const char* target, uint32 len, const char* buf, NPBool file ) {
	TRACE( "PostURL" );
	return NO_ERROR;
}

static NPError RequestRead( NPStream* stream, NPByteRange* rangeList ) {
	TRACE( "RequestRead" );
	return NO_ERROR;
}

static NPError NewStream( NPP instance, NPMIMEType type, const char* target, NPStream** stream ) {
	TRACE( "NewStream" );
	return NO_ERROR;
}

static int32 Write( NPP instance, NPStream* stream, int32 len, void* buffer ) {
	TRACE( "Write" );
	return 0;
}

static NPError DestroyStream( NPP instance, NPStream* stream, NPReason reason ) {
	TRACE( "DestroyStream" );
	return NO_ERROR;
}

static void _InvalidateRect( NPP instance, NPRect *invalidRect ) {
	flash *f = (flash*)instance->ndata;
	system_window_invalidate(flashp_get_window(f),invalidRect);
}

static void InvalidateRegion( NPP instance, NPRegion invalidRegion ) {
	TRACE( "InvalidateRegion" );
}

static void ForceRedraw( NPP instance ) {
	TRACE( "ForceRedraw" );
}

// The host provides a central dictionary for scripting
// identifiers. The folowing methods facilitate access
// to this dictionary:

static NPIdentifier GetStringIdentifier( const NPUTF8 *name ) {
	return addNPId(name);
}

static void GetStringIdentifiers( const NPUTF8 **names, int32_t nameCount, NPIdentifier *identifiers ) {
	SCRIPT_TRACE( "GetStringIdentifiers" );
}

static NPIdentifier GetIntIdentifier( int32_t intid ) {
	SCRIPT_TRACE( "GetIntIdentifier %i", intid );
	return NO_IDENTIFIER;
}

static bool IdentifierIsString( NPIdentifier id ) {
	SCRIPT_TRACE( "IdentifierIsString %i", id );
	return getNPId(id) != NULL;
}

static NPUTF8* UTF8FromIdentifier( NPIdentifier identifier ) {
	const char *result = getNPId(identifier);
	SCRIPT_TRACE( "UTF8FromIdentifier %i", identifier );
	return result ? strdup(result) : NULL;
}

static int32_t IntFromIdentifier( NPIdentifier id ) {
	SCRIPT_TRACE( "IntFromIdentifier %i", id );
	return 0;
}

// The host must facilitate the creation of new script
// objects. In our system there's only a few one host, and
// one flash object per window.
// The host is also responsible for facilitating reference
// counting.

static NPObject* CreateObject( NPP npp, NPClass *aClass ) {
	NPObject *o;
	if( aClass->allocate )
		o = aClass->allocate(npp,aClass);
	else
		o = (NPObject*)malloc(sizeof(NPObject));
	o->_class = aClass;
	o->referenceCount = 1;
	SCRIPT_TRACE("CreateObject %X", o);
    return o;
}

static NPObject *RetainObject( NPObject *npobj ) {
	if( npobj == NULL )
		return NULL;
	npobj->referenceCount++;
	return npobj;
}

// used by flash
void DoReleaseObject( NPObject *npobj ) {
	if( npobj == NULL )
		return;
	npobj->referenceCount--;
	if( npobj->referenceCount != 0 )
		return;
	TRACE("ReleaseObject %X", npobj);
	if( npobj->_class->deallocate ) {
		npobj->_class->deallocate(npobj);
		return;
	}
	if( npobj->_class->invalidate )
		npobj->_class->invalidate(npobj);
	free(npobj);
}

// used by flash
void DoReleaseVariant( NPVariant *variant ) {
	switch( variant->type ) {
	case NPVariantType_Null:
	case NPVariantType_Void:
		break;
	case NPVariantType_String:
		free( (char*)variant->value.stringValue.utf8characters );
		variant->type = NPVariantType_Void;
		break;
	case NPVariantType_Object:
		DoReleaseObject(variant->value.objectValue);
		variant->type = NPVariantType_Void;
		break;
	default:
		SCRIPT_TRACE("ReleaseVariantValue %i", variant->type);
		break;
	}
}

static void SetException( NPObject *npobj, const NPUTF8 *message ) {
	SCRIPT_TRACE("SetException %s",message);
	// Nicolas : in which case is this useful ?
	//throw_exception(message);
}

// NP runtime methods used by Flash to communicate
// with the host's scripting environment. All are
// forwarded to their instance specific handlers:

#define INVOKE_RESPONSE "<invoke name=\"%s\" returntype=\"javascript\"><arguments><null/></arguments></invoke>"

static bool Invoke( NPP npp, NPObject *npobj, NPIdentifier npid, const NPVariant *args, uint32_t argCount, NPVariant *result ) {
	traceObjectOnCall(__FUNCTION__,npobj);

	if( matchNPId(npid,FLASH_REQUEST) && argCount == 1 && args[0].type == NPVariantType_String ) {
		char buf[256];
		sprintf(buf,INVOKE_RESPONSE,args[0].value.stringValue.utf8characters);
		SCRIPT_TRACE(
			"Invoke __flash__request(%s)",
			args[0].value.stringValue.utf8characters
		);
		result->type = NPVariantType_String;
		result->value.stringValue.utf8characters = strdup(buf);
		result->value.stringValue.utf8length = (int)strlen(INVOKE_RESPONSE)-2+args[0].value.stringValue.utf8length+1;
		return 1;
	}
	if( matchNPId(npid,FLASH_REQUEST) && argCount == 3 &&
		args[0].type == NPVariantType_String &&
		args[1].type == NPVariantType_String &&
		args[2].type == NPVariantType_String
	) {
		int size;
		flash *f = (flash*)npp->ndata;
		const char *c;
		SCRIPT_TRACE(
			"Invoke __flash__request(%s,%s,%s)",
			args[0].value.stringValue.utf8characters,
			args[1].value.stringValue.utf8characters,
			args[2].value.stringValue.utf8characters
		);
		flashp_call(f,":request1",args[0].value.stringValue.utf8characters,NULL);
		flashp_call(f,":request2",args[1].value.stringValue.utf8characters,NULL);
		c = flashp_call(f,":request",args[2].value.stringValue.utf8characters,&size);
		if( c == NULL )
			return 0;
		result->type = NPVariantType_String;
		result->value.stringValue.utf8characters = strdup(c);
		result->value.stringValue.utf8length = size;
		return 1;
	}
	if( matchNPId(npid,FSCMD) && argCount == 2 &&
		args[0].type == NPVariantType_String &&
		args[1].type == NPVariantType_String
	) {
		int size;
		flash *f = (flash*)npp->ndata;
		const char *c;
		SCRIPT_TRACE(
			"Invoke _DoFSCommand(%s,%s)",
			args[0].value.stringValue.utf8characters,
			args[1].value.stringValue.utf8characters
		);
		flashp_call(f,":request1",args[0].value.stringValue.utf8characters,NULL);
		c = flashp_call(f,":fscmd",args[1].value.stringValue.utf8characters,&size);
		if( c == NULL )
			return 0;
		result->type = NPVariantType_String;
		result->value.stringValue.utf8characters = strdup(c);
		result->value.stringValue.utf8length = size;
		// result is ignored by flash player
		return 1;
	}
	if( npobj == &__location ) {
		if( matchNPId(npid,"toString") ) {
			flash *f = (flash*)npp->ndata;
			char *location = strdup(flashp_get_location(f));
			int i = (int)strlen(location);
			while(i) {
				if (location[i]=='\\') location[i]='/';
				i--;
			}
			result->type = NPVariantType_String;
			result->value.stringValue.utf8characters = location;
			result->value.stringValue.utf8length = (int)strlen(location);
			SCRIPT_TRACE( "Returned location.toString: %s", location);
		}
		return 1;
	}
	if( npobj == &__top_location ) {
		if( matchNPId(npid,"toString") ) {
			result->type = NPVariantType_String;
			// "chrome://global/content/console.xul" is what Firefox returns for 'top.location.toString()';
			result->value.stringValue.utf8characters = strdup("chrome://global/content/console.xul");
			result->value.stringValue.utf8length = (int)strlen(result->value.stringValue.utf8characters);
			SCRIPT_TRACE("Returned top.location.toString: %s", result->value.stringValue.utf8characters);
		}
		return 1;
	}
	//On OSX, Flash retreives locations by injected functions:
	if( matchNPId(npid,"__flash_getWindowLocation") ) {
		// return the location object:
		result->type = NPVariantType_Object;
		result->value.objectValue = &__location;
		RetainObject(&__location);
		return 1;
	}
	if( matchNPId(npid,"__flash_getTopLocation") ) {
		// return the top_location object:
		result->type = NPVariantType_Object;
		result->value.objectValue = &__top_location;
		RetainObject(&__top_location);
		return 1;
	}
	SCRIPT_TRACE( "Unhandled 'Invoke', npid: %i", npid );
	return 0;
}

static bool InvokeDefault( NPP npp, NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	SCRIPT_TRACE( "InvokeDefault" );
	return 0;
}

static bool GetProperty( NPP npp, NPObject *npobj, NPIdentifier npid, NPVariant *result ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	if (npobj == &__window) {
		if( matchNPId(npid,"location") ) {
			result->type = NPVariantType_Object;
			result->value.objectValue = &__location;
			RetainObject(&__location);
			SCRIPT_TRACE("Returned 'location' class");
			return 1;
		}
		if( matchNPId(npid,"top") ) {
			result->type = NPVariantType_Object;
			result->value.objectValue = &__top;
			RetainObject(&__top);
			SCRIPT_TRACE("Returned 'top' class");
			return 1;
		}
	} else if (npobj == &__top) {
		if ( matchNPId(npid,"location") ) {
			result->type = NPVariantType_Object;
			result->value.objectValue = &__top_location;
			RetainObject(&__top_location);
			SCRIPT_TRACE("Returned 'top.location' class");
			return 1;
		}
	}
	return 0;
}

static bool SetProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName, const NPVariant *value ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	SCRIPT_TRACE( "SetProperty %i", propertyName );
	return 0;
}

static bool RemoveProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	SCRIPT_TRACE( "RemoveProperty %i", propertyName );
	return 0;
}

static bool HasProperty( NPP npp, NPObject *npobj, NPIdentifier propertyName ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	SCRIPT_TRACE( "HasProperty %i", propertyName );
	return 0;
}

static bool HasMethod( NPP npp, NPObject *npobj, NPIdentifier methodName ) {
	traceObjectOnCall(__FUNCTION__,npobj);
	SCRIPT_TRACE( "HasMethod %i", methodName );
	return 0;
}

static int unescape( char *str, int *ssize ) {
	int k = 0, esc = 0;
	// UNESCAPE the string
	while( k < *ssize ) {
		if( !esc ) {
			if( str[k++] == '\\' )
				esc = 1;
		} else {
			char c;
			switch( str[k] ) {
			case '"': c = '"'; break;
			case '\\': c = '\\'; break;
			case 'r': c = '\r'; break;
			case 'n': c = '\n'; break;
			case 't': c = '\t'; break;
			default:
				return 0;
			}
			(*ssize)--;
			memcpy(str+k,str+k+1,*ssize-k);
			str[k-1] = c;
			esc = 0;
		}
	}
	str[*ssize] = 0;
	return 1;
}

static const char *end_of_string( const char *send ) {
	int esc = 0;
	while( *send ) {
		switch( *send ) {
		case '"':
			if( !esc )
				return send;
			esc = 0;
			break;
		case '\\':
			esc = !esc;
			break;
		default:
			esc = 0;
			break;
		}
		send++;
	}
	return NULL;
}

#define JS_CALL_START "try { __flash__toXML("
#define JS_RESULT_START "var __flash_temp = \""

static bool Evaluate( NPP npp, NPObject *npobj, NPString *script, NPVariant *result ) {
	SCRIPT_TRACE( "Evaluate %s", script->utf8characters );
	if( memcmp(script->utf8characters,JS_CALL_START,strlen(JS_CALL_START)) == 0 ) {
		const char *p = script->utf8characters + strlen(JS_CALL_START);
		const char *s = p;
		const char *send;
		while( *s && *s != '(' )
			s++;
		if( !*s || s[1] != '"' )
			return 0;
		s += 2;
		send = end_of_string(s);
		if( send == NULL || send[1] != ')' )
			return 0;
		{
			int isize = (int)(s - p) - 2;
			int ssize = (int)(send - s);
			char *ident = (char*)malloc(isize+1);
			char *str = (char*)malloc(ssize+1);
			memcpy(ident,p,isize);
			memcpy(str,s,ssize);
			ident[isize] = 0;
			if( !unescape(str,&ssize) ) {
				free(ident);
				free(str);
				return 0;
			}
			// CALLBACK
			{
				flash *f = (flash*)npp->ndata;
				int size = 0;
				const char *res = flashp_call(f,ident,str,&size);
				free(ident);
				free(str);
				if( res == NULL )
					return 0;
				result->type = NPVariantType_String;
				result->value.stringValue.utf8characters = strdup(res);
				result->value.stringValue.utf8length = size;
				return 1;
			}
		}
	}
	if( memcmp(script->utf8characters,JS_RESULT_START,strlen(JS_RESULT_START)) == 0 ) {
		const char *s = script->utf8characters + strlen(JS_RESULT_START);
		const char *send = end_of_string(s);
		char *str;
		int ssize;
		if( send == NULL || send[1] != ';' )
			return 0;
		ssize = (int)(send - s);
		str = (char*)malloc(ssize+1);
		memcpy(str,s,ssize);
		if( !unescape(str,&ssize) ) {
			free(str);
			return 0;
		}
		result->type = NPVariantType_String;
		result->value.stringValue.utf8characters = str;
		result->value.stringValue.utf8length = ssize;
		return 1;
	}
	result->type = NPVariantType_Void;
	return 1;
}

void getHostTable(NPNetscapeFuncs *np_host_table) {
	static NPNetscapeFuncs table = {
		sizeof(NPNetscapeFuncs)
		, (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR
		, GetURL
		, PostURL
		, RequestRead
		, NewStream
		, Write
		, DestroyStream
		, Status_
		, UserAgent
		, malloc
		, free
		, MemFlush
		, ReloadPlugins
		, GetJavaEnv
		, GetJavaPeer
		, GetURLNotify
		, PostURLNotify
		, GetValue
		, SetValue
		, _InvalidateRect
		, InvalidateRegion
		, ForceRedraw
		, GetStringIdentifier
		, GetStringIdentifiers
		, GetIntIdentifier
		, IdentifierIsString
		, UTF8FromIdentifier
		, IntFromIdentifier
		, CreateObject
		, RetainObject
		, DoReleaseObject
		, Invoke
		, InvokeDefault
		, Evaluate
		, GetProperty
		, SetProperty
		, RemoveProperty
		, HasProperty
		, HasMethod
		, DoReleaseVariant
		, SetException
	};
	memcpy(np_host_table,&table,table.size);
}
