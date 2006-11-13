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

#include <neko/neko_vm.h>

#define DEFAULT_INDEX "app.n"
static int boot_main(int argc, char **argv);

char ** switches = NULL;
long switches_count = 0;

#if _WIN32

#include <windows.h>
#include <direct.h>

void parseCmdLine(LPSTR lpCmdLine, int *argc, char **argv[]) {
	if (lpCmdLine && *lpCmdLine) {
		size_t len = 0;
		int c = *argc;
		char* del = 0;
		if(*lpCmdLine == 0x20) {
			// blank space:
			lpCmdLine++;
			len = strspn(lpCmdLine," ");
		} else {
			// space, quote or zero delimited argument:
			len		= *lpCmdLine=='"'
					? strcspn(++lpCmdLine,del="\"")
					: strcspn(lpCmdLine,del=" ");
			if (len>0) {
				*argv = realloc(*argv,(c+1)*sizeof(char*));
				(*argv)[c] = malloc(len+1);
				(*argv)[c][len] = 0;
				strncpy((*argv)[c],lpCmdLine,len);
				c++;
			}
			if (del[0]=='"' && lpCmdLine[len]!=0) lpCmdLine++;
		}
		*argc = c;
		lpCmdLine += len;
		parseCmdLine(lpCmdLine,argc,argv);
	}
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int result;

	// parse command line:
	int argc = 0;
	char** argv = 0;
	parseCmdLine(GetCommandLine(),&argc,&argv);

	// call the main function:
	result = boot_main(argc, argv);

	// clean up parsed arguments:
	while(argc) free(argv[--argc]);
	free(argv);

	// dispatch result:
	return result;
}

#elif OSX

#define stricmp strcasecmp
#include <Carbon/Carbon.h>

int main (int argc, char ** argv) {
	return boot_main(argc, argv);
}

char* getBundleRoot(){
	// Adjust the path to the folder containing .app file:
	CFBundleRef br;
	CFURLRef ur;
	CFStringRef sr;

	br = CFBundleGetMainBundle();
	if (br) {
		long i;
		ur = CFBundleCopyBundleURL(br);
		sr = CFURLCopyFileSystemPath(ur, kCFURLPOSIXPathStyle);
		i = switches_count++;
		switches = realloc(switches,switches_count*sizeof(char*));
		switches[i] = malloc(FILENAME_MAX);
		CFStringGetCString(sr,switches[i],FILENAME_MAX,kCFStringEncodingASCII);
		CFRelease(ur);
		CFRelease(sr);
		return switches[i];
	}
	return 0;
};

#endif

char *getSwitch(int argc, char *args[], char *swtch) {
	int i = 1;
	while( argc-i-1 > 0 ) {
		if (args[i][0]=='-')
			if (!stricmp(args[i]+1,swtch))
				return args[i+1];
		i++;
	}
	return NULL;
}

char *getSwitchFromBundle(char *swtch) {	
#if OSX
	{
		CFBundleRef br = CFBundleGetMainBundle();
		if (br) {
			CFStringRef cfswitch = CFStringCreateWithCString(0,swtch,kCFStringEncodingASCII);
			CFStringRef bd_index = CFBundleGetValueForInfoDictionaryKey(br,cfswitch);
			if (bd_index) {
				long i=switches_count++;
				switches = realloc(switches,switches_count*sizeof(char*));
				switches[i] = malloc(FILENAME_MAX);
				CFStringGetCString(bd_index,switches[i],FILENAME_MAX,kCFStringEncodingASCII);
				return switches[i];
			}
		}
	}
#endif
	return NULL;
}

static void report( neko_vm *vm, value exc ) {
#if OSX
	CFStringRef title = CFSTR("Uncaught exception");
	CFStringRef message;
#endif
	int i = 0;
	buffer b = alloc_buffer(NULL);
	value st = neko_exc_stack(vm);
	if( val_array_size(st) > 20 ) {
		i = val_array_size(st) - 20;
		buffer_append(b,"...\n");
	}
	for(i;i<val_array_size(st);i++) {
		value s = val_array_ptr(st)[i];
		if( val_is_null(s) )
			buffer_append(b,"Called from a C function\n");
		else if( val_is_string(s) ) {
			buffer_append(b,"Called from ");
			buffer_append(b,val_string(s));
			buffer_append(b," (no debug available)\n");
		} else if( val_is_array(s) && val_array_size(s) == 2 && val_is_string(val_array_ptr(s)[0]) && val_is_int(val_array_ptr(s)[1]) ) {
			buffer_append(b,"Called from ");
			buffer_append(b,val_string(val_array_ptr(s)[0]));
			buffer_append(b," line ");
			val_buffer(b,val_array_ptr(s)[1]);
			buffer_append(b,"\n");
		} else {
			buffer_append(b,"Called from ");
			val_buffer(b,s);
			buffer_append(b,"\n");
		}
	}
	val_buffer(b,exc);
#if _WIN32
	MessageBox(NULL,val_string(buffer_to_string(b)),"Uncaught exception",MB_OK | MB_ICONERROR);
#elif OSX
	message = CFStringCreateWithCString(NULL,val_string(buffer_to_string(b)), kCFStringEncodingUTF8);
	CFUserNotificationDisplayNotice(0,0,NULL,NULL,NULL,title,message,NULL);
#endif
}

#ifdef OSX
char *system_fullpath( const char *file ) {
	char path[2048];
	FSRef ref;
	FSPathMakeRef ((const UInt8*) file, &ref, 0 );
	FSRefMakePath ( &ref, (UInt8*) path, 2048 );
	return strdup(path);
}
#endif

static int boot_main(int argc, char *argv[] ) {	
	neko_vm *vm;
	value args[2];
	value mload, exc = NULL;

	char* root = getSwitch(argc,argv,"swroot");
	char* rootFromBundle = root ? NULL : getSwitchFromBundle("swroot");
	
	char* index = getSwitch(argc,argv,"swindex");
	if(!index) index = getSwitchFromBundle("swindex");
	
#if OSX	
	if (rootFromBundle) {
		if (stricmp("SW_BUNDLE_PARENT",rootFromBundle)==0) {
			// folder containing bundle is path:
			root = getBundleRoot();
			strcat(root,"/..");
		} else {
			// path is relative to bundle:
			char *rel = strdup(rootFromBundle);
			sprintf(root,"%s/%s",getBundleRoot(),rel);
			free(rel);
		}
	}
#endif

	// if root folder is specified, change the current directory:
	if( root ) chdir(root);
	// printf("boot-loader computed working folder: %s\n",root);
	// printf("boot-loader set working folder: %s\n",getcwd(NULL));

	// initialize Neko Virtual Machine
	neko_global_init(&vm);
	vm = neko_vm_alloc(NULL);
	neko_vm_jit(vm,1);
	neko_vm_select(vm);
	mload = neko_default_loader(argv, argc);

	args[0] = alloc_string(index ? index : DEFAULT_INDEX);
	args[1] = mload;
	val_callEx(mload,val_field(mload,val_id("loadmodule")),args,2,&exc);
	if( exc != NULL )
		report(vm,exc);
	vm = NULL;
	neko_global_free();

	while(switches_count--) {
		free(switches[switches_count]);
	}
	if (switches) free(switches);

	return( exc != NULL );
}

