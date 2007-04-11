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
package swhx {

	public class Deserializer {

		private var buf : String;
		private var pos : int;
		private var length : int;
		private var cache : Array;
		private var scache : Array;

		static private var reEscBackslash: RegExp = /\\\\/g;
		static private var reEscNewline: RegExp = /\\n/g;
		static private var reEscReturn: RegExp = /\\r/g;
		static private var delim: String = "##__delim__##";
		static private var reDelim: RegExp = /##__delim__##/g;

		public function Deserializer(buf: String) {
			this.buf = buf;
			length = buf.length;
			pos = 0;
			scache = new Array();
			cache = new Array();
		}

		private function readDigits(): Number {
			var k:int = 0;
			var s:Boolean = false;
			var fpos:int = pos;
			while( true ) {
				var c:Number = buf.charCodeAt(pos);
				if( isNaN(c) )
					break;
				if( c == 45 ) { // negative sign
					if( pos != fpos )
						break;
					s = true;
					pos++;
					continue;
				}
				c -= 48;
				if( c < 0 || c > 9 )
					break;
				k = k * 10 + c;
				pos++;
			}
			if( s )
				k *= -1;
			return k;
		}

		public function deserializeObject(o:*,isHash:Boolean):void {
			var e:int = isHash?104:103; /* h or g */
			while( true ) {
				if( pos >= length )
					throw "Flash deserializer: Invalid object";
				if( buf.charCodeAt(pos) == e )
					break;
				var k:* = deserialize();
				if( (typeof k) != "string" )
					throw "Flash deserializer: Invalid object key";
				var v:* = deserialize();
				o[k] = v;
			}
			pos++;
		}

		public function deserialize() : * {
			var c : int, s : String, n : uint;
			switch( buf.charCodeAt(pos++) ) {
			case 110: // n
				return null;
			case 116: // t
				return true;
			case 102: // f
				return false;
			case 122: // z
				return 0;
			case 105: // i
				return readDigits();
			case 100: // d
				var p1 : int = pos;
				while( true ) {
					c = buf.charCodeAt(pos);
					// + - . , 0-9
					if( (c >= 43 && c < 58) || c == 101 /*e*/ || c == 69 /*E*/ )
						pos++;
					else
						break;
				}
				s = buf.substr(p1,pos-p1);
				var f : Number = parseFloat(s);
				if( isNaN(f) )
					throw ("Flash deserializer Invalid float "+s);
				return f;
			case 107: // k
				return Number.NaN;
			case 109: // m
				return Number.NEGATIVE_INFINITY;
			case 112: // p
				return Number.POSITIVE_INFINITY;
			case 121: // y
				var len : int = readDigits();
				if( buf.charAt(pos++) != ":" || length - pos < len )
					throw "Flash deserializer: Invalid string length";
				s = buf.substr(pos,len);
				s = decodeURIComponent(s);
				pos += len;
				scache.push(s);
				return s;
			case 97: // a
				var a:Array = new Array();
				cache.push(a);
				while( true ) {
					if( pos >= length )
						throw "Flash deserializer: Invalid array";
					c = buf.charCodeAt(pos);
					if( c == 104 ) { /*h*/
						pos++;
						break;
					}
					if( c == 117 ) { /*u*/
						pos++;
						n = readDigits();
						a[a.length+n-1] = null;
					} else
						a.push(deserialize());
				}
				return a;
			case 98: // b
				var o1:Object = new Object();
				cache.push(o1);
				deserializeObject(o1,true);
				return o1;
			case 111: // o
				var o2:Object = new Object();
				cache.push(o2);
				deserializeObject(o2,false);
				return o2;
			case 114: // r
				n = readDigits();
				if( n >= cache.length )
					throw "Flash deserializer: Invalid reference";
				return cache[n2];
			case 82: // R
				n = readDigits();
				if( n >= scache.length )
					throw "Flash deserializer: Invalid string reference";
				return scache[n];
			case 120: // x
				throw deserialize();
			case 99: // c
				throw "Flash deserializer: cannot handle classes";
			case 119: // w
				throw "Flash deserializer: cannot handle enumerations";
			// deprecated
			case 115: // s
				n = readDigits();
				if( buf.charAt(pos++) != ":" || length - pos < n )
					throw "Flash deserializer: Invalid string length";
				s = buf.substr(pos,n);
				pos += n;
				scache.push(s);
				return s;
			case 106: // j
				n = readDigits();
				if( buf.charAt(pos++) != ":" )
					throw "Flash deserializer: Invalid string length";
				s = buf.substr(pos,n);
				pos += n;
				s = s.replace(reEscBackslash,delim).replace(reEscReturn,"\r").replace(reEscNewline,"\n").replace(reDelim,"\\");
 				scache.push(s);
				return s;
			default:
			}
			pos--;
			throw("Flash deserializer: Invalid char "+buf.charAt(pos)+" at position "+pos);
		}

		public static function run( v : String ) : Object {
			return (new Deserializer(v)).deserialize();
		}
}

}