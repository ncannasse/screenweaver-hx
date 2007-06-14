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
This class can be used with haXe back-end code (that compiles to Neko byte-code), as well as haXe front-end code (that compiles to SWF).
In the latter case, it is used to connect to back-end code, and vice verse in the first case.
</p>
*/
class Connection extends haxe.remoting.Connection {

	override function __resolve(field) : haxe.remoting.Connection {
		var s = new Connection(__data,__path.copy());
		s.__path.push(field);
		return s;
	}

	/**
	<p>
	Neko usage (back-end): invoke a function with the specified arguments on the front-end.
	</p>
	<p>
	haXe usage (front-end): invoke a function with the specified arguments on the back-end
	</p>
	*/
	override public function call( params : Array<Dynamic> ) : Dynamic {
	#if flash
		var p = __path.copy();
		var f = p.pop();
		var path = p.join(".");
		var s = new haxe.Serializer();
		s.serialize(params);
		var params = haxe.remoting.Connection.escapeString(s.toString());
		var s = flash.external.ExternalInterface.call(path+"."+f,params);
		if( s == null )
			throw "Failed to call Neko method "+__path.join(".");
		return new haxe.Unserializer(s).unserialize();
	#else neko
		var s = new haxe.Serializer();
		s.serialize(params);
		var r = __data.doCall(__path.join("."),s.toString(),null);
		if( r == null )
			throw "Failed to call Flash method "+__path.join(".");
		return new haxe.Unserializer(r).unserialize();
	#else error
	#end
	}

#if flash
	static function doCall( path : String, params : String ) : String {
		var p = path.split(".");
		var f = p.pop();
		return haxe.remoting.Connection.doCall(p.join("."),f,params);
	}

	static function __init__() {
	#if flash9
		flash.external.ExternalInterface.addCallback("swhxCall",doCall);
	#else flash
		flash.external.ExternalInterface.addCallback("swhxCall",null,doCall);
	#end
	}

	/**
	<p>
	haXe (front-end) only: initialize connection to the back-end
	</p>
	*/
	public static function desktopConnect() {
		if( !flash.external.ExternalInterface.available )
			throw "External Interface not available";
		if( flash.external.ExternalInterface.call(":desktop",":available") != "yes" )
			throw "Could not connect to the Desktop";
		return new Connection(null,[]);
	}

#else neko

	/**
	<p>
	Neko (back-end) only: initialize connection to the specified front-end (UI)
	</p>
	*/
	public static function flashConnect( flash : Flash ) {
		return new Connection(untyped flash,[]);
	}

#end

}
