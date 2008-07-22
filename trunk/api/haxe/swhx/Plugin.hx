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

class Plugin {

	public static var PLAYER = "flashplayer.bin";

	public static var URLS = {
		Windows : "http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-win.xpi",
		Mac : "http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-mac.xpi",
		Linux : "http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-linux.xpi"
	};

	public static var LINUX_SO = "libflashplayer.so";
	public static var WIN32_DLL = "NPSWF32.dll";
	public static var OSX_BUNDLE_PATH = "/Contents/MacOS/Flash Player";
	public static var OSX_ZIP_PATH = "plugin/Flash Player.plugin";

	public static var SEARCH = {
		Windows: [
			neko.Sys.getEnv("ProgramFiles") + "\\Mozilla Firefox\\plugins\\NPSWF32.dll",
			neko.Sys.getEnv("SystemRoot") + "\\System32\\Macromed\\Flash\\NPSWF32.dll" ,
		],
		Mac : [ "/Library/Internet Plug-Ins/Flash player.plugin" ],
		Linux: [
                        "~/.mozilla/plugins/libflashplayer.so",
                        "/usr/lib/nsbrowser/plugins/libflashplayer.so",
                        "/usr/lib/mozilla/plugins/libflashplayer.so",
                        "/usr/lib/firefox/plugins/libflashplayer.so",
                ],
	};

	public static function find(?_version: Int) : String {
		var system = neko.Sys.systemName();
		var path = PLAYER;

		if( system == "Mac" )
			path += OSX_BUNDLE_PATH;

		// look in default location first:
		var version : String = _fileversion(untyped path.__s);
		// don't check the version : this should be the one we downloaded
		if( version != null )
			return path;

		// search for a pre-installed player:
		var search_list: Array<String> = Reflect.field(SEARCH,system);
		for( p in search_list ) {
			var version = new String(_fileversion(untyped p.__s));
			if( version != null ) {
				var ord_version = Std.parseInt(version.substr(0,version.indexOf(".")));
				var min_version = if (_version == null) 8; else _version;
				if( min_version <= ord_version ) {
					if( system == "Mac" )
						p += OSX_BUNDLE_PATH;
					return p;
				}
			}
		}

		// try to download a player:
		update();
		return path;
	}

	public static function update() {
		var sysname = neko.Sys.systemName();
		if( neko.FileSystem.exists(PLAYER) )
			return;
		try {
			var url = Reflect.field(URLS,sysname);
			var data = haxe.Http.request(url);
			var zip = neko.zip.Reader.readZip(new haxe.io.StringInput(data));
			if( sysname == "Windows" ) {
				for( file in zip ) {
					if( file.fileName == WIN32_DLL ) {
						var data = neko.zip.Reader.unzip(file);
						var f = neko.io.File.write(PLAYER,true);
						f.write(data);
						f.close();
						return;
					}
				}
				throw WIN32_DLL+" not found in "+url;
			}
			// on OSX, unzip the whole directory recursively
			var ol =  OSX_ZIP_PATH.length;
			for( file in zip ) {
				var name = file.fileName;
				if( name.length <= ol || name.substr(0,ol) != OSX_ZIP_PATH )
					continue;
				name = PLAYER + name.substr(ol,name.length - ol);
				if( name.charAt(name.length-1) == "/" ) {
					neko.FileSystem.createDirectory(name);
					continue;
				}
				var data = neko.zip.Reader.unzip(file);
				var f = neko.io.File.write(name,true);
				f.write(data);
				f.close();
			}

		} catch( e : Dynamic ) {
			throw "An error occured while updating the Flash plugin ("+Std.string(e)+")";
		}
	}

	static var _fileversion = neko.Lib.load("swhx","plugin_file_version",1);

}
