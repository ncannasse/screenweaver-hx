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
	The Flash class provides control over the content being played back in a Screenweaver HX window.
**/

class Flash {

	var f : Void;
	var server : neko.net.RemotingServer;
	var loaded: Bool;
	var initialized : Bool;

	/**
	<p>
	Create a new Flash instance.
	A Flash instance requires a host window of type swhx.Window.
	The remoting server argument can be "null" in case no calls will be made from the front-end to the back-end.
	</p>
	*/
	public function new( w : Window, s : neko.net.RemotingServer ) {
		f = _flash_new(untyped w.w);
		server = s;
		var me = this;
		_flash_on_call(f,function(ident,params) {
			var r = me.onCall(new String(ident),new String(params));
			if( r == null )
				return null;
			if( !Std.is(r,String) )
				throw "Not a string";
			return untyped r.__s;
		},function(s,url,post) {
			// there's a maximum number of concurrent threads set by the GC
			// if reached, it might be useful to write some thread manager
			neko.vm.Thread.create(callback(me.onGetURL,new Stream(s),new String(url),if( post == null ) null else new String(post)));
		});
		me.loaded = false;
	}

	function doCall( ident : String, params : String, ret : String -> Void ) : String {
		if( !initialized )
			throw "The SWF is not yet loaded or has not been compiled with swhx.Connection class (or swhx.Api.init has not been called for AS2/AS3 project)";
		var me = this;
		return neko.vm.Ui.syncResult(function() {
			var r = _flash_call(me.f,untyped ident.__s,untyped params.__s);
			if( r != null )
				r = new String(r);
			if( ret != null )
				ret(r);
			return r;
		});
	}

	static var save1 : String;
	static var save2 : String;
	static var postData : String;

	function onCall( ident : String, params : String ) : String {
		// initialization
		if( ident == ":desktop" && params == ":available" )
			return escape("yes");
		if( ident == ":init" && params == "" ) {
			initialized = true;
			return "";
		}

		if( ident == ":request1" ) {
			save1 = params;
			return "";
		}
		if( ident == ":request2" ) {
			save2 = params;
			return "";
		}
		if( ident == ":request" ) {
			var s1 = save1;
			var s2 = save2;
			save1 = null;
			save2 = null;
			return '<invoke name="'+s1+'" returntype="javascript"><arguments>'+escape(s2)+escape(params)+'</arguments></invoke>';
		}
		if( ident == ":fscmd" ) {
			var p = save1;
			save1 = null;
			return onFSCommand(p,params);
		}

		try {
			var p = ident.split(".");
			var fname = p.pop();
			var o = server.resolvePath(p);
			var f = Reflect.field(o,fname);
			if( f == null )
				throw "Unknown method "+ident;
			var args = new haxe.Unserializer(params).unserialize();
			var r = Reflect.callMethod(o,f,args);
			var s = new haxe.Serializer();
			s.serialize(r);
			return escape(s.toString());
		} catch( e : Dynamic ) {
			var s = new haxe.Serializer();
			s.serializeException(e);
			return escape(s.toString());
		}
	}

	static function escape( r : String ) {
		var r = r.split("&").join("&amp;").split("<").join("&lt;").split(">").join("&gt;").split('"').join("&quot;").split("'").join("&apos;");
		r = "<string>"+r+"</string>";
		return r;
	}

	/**
	<p>
	Internal method that handles URL requests originating from the front-end.
	This method can be overidden to register, filter or block front-end URL requests.
	Overrides that do not wish to block data retreival should invoke ''super.onGetURL(...)''.
	</p>
	*/
	public function onGetURL( s : Stream, url : String, postData : String) : Void {
		try {
			if( url.substr(0,7) == "http://" ) {
				var h = new haxe.Http(url);
				h.onError = function(e) { s.reportError(); }
				if( postData != null )
					untyped h.postData = postData;
				h.customRequest(postData != null,s);
				return;
			}
			if( postData != null )
				throw "Cannot post without http://";
			if(	url.substr(0,8) == "file:///") {
				url = url.substr(8,url.length-8);
				url = StringTools.urlDecode(url);
			}
			var f;
			var size;
			try {
				size = neko.FileSystem.stat(url).size;
				f = neko.io.File.read(url,true);
			} catch( e : Dynamic ) {
				s.reportError();
				return;
			}
			s.prepare(size);
			var bufsize = (1 << 16); // 65K
			var buf = neko.Lib.makeString(bufsize);
			while( size > 0 ) {
				var bytes = f.readBytes(buf,0,bufsize);
				if( bytes <= 0 ) {
					f.close();
					s.reportError();
					return;
				}
				size -= bytes;
				var pos = 0;
				while( bytes > 0 ) {
					var k = s.writeBytes(buf,pos,bytes);
					if( k <= 0 ) {
						f.close();
						s.reportError();
						return;
					}
					pos += k;
					bytes -= k;
				}
			}
			s.close();
			f.close();
			if (!loaded) {
				loaded = true;
				neko.vm.Ui.sync(onSourceLoaded);
			}
		} catch( e : Dynamic ) {
			s.reportError();
		}
	}

	/**
	<p>
	Callback function that will be triggered upon the front-end invoking an fscommand.
	Override to add handlers for specific command strings. Overrides are not required to invoke "super.onFSCommand(...)".
	</p>
	*/
	public function onFSCommand( cmd : String, param : String ) : String {
		throw ("Unknown FSCommand: "+cmd+" arguments: "+param);
		return null;
	}

	/**
	<p>
	Starts Flash movie play-back.
	</p>
	*/
	public function start() {
		_flash_start(f);
	}

	/**
	<p>
	Stops Flash movie play-back and destoys the Flash player plug-in instance.
	</p>
	*/
	public function destroy() {
		_flash_destroy(f);
	}

	/**
	<p>
	Returns the value of the indicated "EMBEDDED" attribute.
	</p>
	*/
	public function getAttribute( att : String ) {
		var s = _flash_get_attribute(f,untyped att.__s);
		if( s == null )
			return null;
		return new String(s);
	}

	/**
	<p>
	Sets the value of the indicated "EMBEDDED" attribute.
	Attributes are used at front-end start-up, so settings attributes on a swhx.Flash instance after swhx.Flash.start() has been called, has no effect.
	</p>
	*/
	public function setAttribute( att : String, value : String ) {
		_flash_set_attribute(f,untyped att.__s,untyped value.__s);
	}

	/**
	<p>
	Callback function that will be triggered when Screenweaver is finished loading the Flash movie specified by the "SRC" attribute.
	Movie loading is initiated upon invoking swhx.Flash.start().
	Override this method to do processing that requires the front-end Flash movie to be in a running state, such as calling functions on the front-end using swhx.Connection.
	</p>
	*/
	public function onSourceLoaded(): Void {
	}

	static var _flash_new = neko.Lib.load("swhx","flash_new",1);
	static var _flash_get_attribute = neko.Lib.load("swhx","flash_get_attribute",2);
	static var _flash_set_attribute = neko.Lib.load("swhx","flash_set_attribute",3);
	static var _flash_start = neko.Lib.load("swhx","flash_start",1);
	static var _flash_destroy = neko.Lib.load("swhx","flash_destroy",1);
	static var _flash_on_call = neko.Lib.load("swhx","flash_on_call",3);
	static var _flash_call = neko.Lib.load("swhx","flash_call",3);

}
