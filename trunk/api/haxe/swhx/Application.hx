/*
 * Copyright (c) 2006, Edwin van Rijkom, Nicolas Cannasse
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE HAXE PROJECT CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE HAXE PROJECT CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
package swhx;

/**
<p>
The Application class provides for Screenweaver HX application wide operations.
The class can not be instantiated because an SWHX application cannot run more than one message loops at a time.
Once swhx.Application.loop() has been invoked, all further back-end code can only be invoked by means of callbacks.
</p>
**/

class Application {
	static var path: String;

	/**
	<p>
	Initializes the shwx.ndll Neko extension.

	Optionally, SWHX will attempt to load the Flash player Netscape plug-in from the path specified.
	When no path is specified, SWHX will look for the plug-in (version 8 or 9) at /Library/Internet Plug-Ins on OS-X,
	and %PROGRAM_FILES%\Mozilla Firefox\plugins on Windows.
	A version argument can be passed to indicate that any pre-installed plug-ins found on the system should be by-passed in case they are of lower version than specified.

	</p>
	<p>
	In case the plug-in is not found in any of the forementioned locations, SWHX will download the plugin from one of the following URLs:
	</p>
	<p>
	http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-win.xpi
	http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-mac.xpi
	</p>
	<p>
	Downloading can be disabled by specifying -d NO_SWHX_PLUGIN on compilation of the back-end.
	</p>
	**/
	public static function init( ?_version: Int, ?_path : String ) {
		path = _path;
		#if !NO_SWHX_PLUGIN
		if( path == null )
			path = Plugin.find(_version);
		#end
		if (path == null)
			throw("Path to Flash player is not specified");
		_initialize(untyped path.__s);
	}

	/**
	Starts the application message loop.
	This call will not return until all application windows have been closed.
	**/
	public static function loop() {
		neko.vm.Ui.loop();
	}

	/**
	Exit the application message loop. Issuing this call will result in all SWHX windows closing.
	**/
	public static function exitLoop() {
		neko.vm.Ui.stopLoop();
	}

	/**
	Clean up the resources allocated during initialization.
	**/
	public static function cleanup() {
		_cleanup();
	}

	static var _initialize = neko.Lib.load("swhx","initialize",1);
	static var _cleanup = neko.Lib.load("swhx","cleanup",0);
}