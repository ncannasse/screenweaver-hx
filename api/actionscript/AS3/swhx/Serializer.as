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

	import flash.utils.ByteArray;

	public class Serializer{

		/* prefixes :
		a : array
		b : hash		(not supported)
		c : class		(not supported)
		d : Float
		e : reserved (float exp)
		f : false
		g : object end
		h : array/list/hash end
		i : Int
		j : (deprecated)
		k : NaN
		l : list		(not supported)
		m : -Inf
		n : null
		o : object
		p : +Inf
		q : inthash		(not supported)
		r : reference
		s : (deprecated)
		t : true
		u : array nulls
		v : date		(not supported)
		w : enum		(not supported)
		x : exception
		y : urlencoded string
		z : zero
		*/

		private var buf: String;
		private var ocache: Array;
		private var scache: Array;

		static private var reBackslash: RegExp = /\\/g;
		static private var reNewline: RegExp = /\n/g;
		static private var reReturn: RegExp = /\r/g;

		public function Serializer() {
			buf = "";
			ocache = new Array();
			scache = new Array();
		};

		public function dontUseCache():void {
			this.ocache = null;
		};

		public function serialize(value:*):void {
			// null
			if (value == null) {
				this.buf += "n";
				return;
			}

			// Number
			if (value is Number) {
				if (isNaN(value)) {
					// Not a number
					this.buf += "k";
					return;
				}
				if (value == Number.POSITIVE_INFINITY) {
					this.buf += "p";
					return;
				}
				if (value == Number.NEGATIVE_INFINITY) {
					this.buf += "m";
					return;
				}
				var str:String = value.toString();
				// Float detection: convert to string and see if there's a dot...
				if (str.indexOf(".",0) != -1) {
					// Float
					this.buf += "d";
					this.buf += str;
					return;
				} else {
					// Int
					if (value == 0) {
						this.buf += "z";
						return;
					}
					this.buf += "i";
					this.buf += str;
					return;
				}
			}

			// String
			if (value is String) {
				this.serializeString(value);
				return;
			}

			// Array
			if (value is Array) {
				if (this.serializeRef(value)) {
					return;
				}
				this.buf += "a";
				var i:int = 0;
				var l:int = value.length;
				var c:int = 0;
				while (c < l) {
					if (value[c] != null) {
						if (i > 0) {
							if (i != 1) {
								this.buf += "u";
								this.buf += i;
							} else {
								this.buf += "n";
							}
							i = 0;
						}
						this.serialize(value[c]);
					} else {
						i++;
					}
					c++;
				}
				if (i > 0) {
					if (i != 1) {
						this.buf += "u";
						this.buf += i;
					} else {
						this.buf += "n";
					 }
				}
				this.buf += "h";
				return;
			}

			// Boolean true
			if (value == true) {
				this.buf += "t";
				return;
			}

			// Boolean false
			if (value == false) {
				this.buf += "f";
				return;
			}

			// Reference
			if (this.serializeRef(value)) {
				return;
			}

			// Function
			if (value is Function) {
				throw "Flash serializer Cannot serialize function: '"+value+"'\n";
			}

			// ByteArray
			if( value is flash.utils.ByteArray ) {
				this.buf += "y";
				var s : String = "";
				var b : flash.utils.ByteArray = value;
				var CHARS:Array = ["0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"];
				var size : uint = b.length;
				for(var p:uint=0;p<size;p++) {
					var c1:uint = b[p];
					// alphanum
					if( (c1 >= 48 && c1 <= 57) || (c1 >= 65 && c1 <= 90) || (c1 >= 97 && c1 <= 122) )
						s += String.fromCharCode(c1);
					else
						s += "%"+CHARS[c1>>4]+CHARS[c1&15];
				}
				this.buf += s.length;
				this.buf += ":";
				this.buf += s;
				return;
			}

			// Object
			if (value is Object) {
				this.buf += "o";
				for(var k:String in value) {
					this.serializeString(k);
					this.serialize(value[k]);
				}
				this.buf += "g";
				return;
			}

			// shouldn't reach this:
			throw "Flash deserializer Unknown value type";
		};

		public function serializeException(value:*):void {
			this.buf += "x";
			this.serialize(value);
		};

		public function serializeRef(value:*): Boolean {
			if (this.ocache == null) {
				return (false);
			}
			var i:int = 0;
			var l:int = this.ocache.length;
			while (i < l) {
				if (this.ocache[i] == value) {
					this.buf += "r";
					this.buf += i;
					return (true);
				}
				i++;
			}
			this.ocache.push(value);
			return false;
		};

		public function serializeString(value: String):void {
			var r:* = null;
			var l:int = this.scache.length;
			for (var i:int=0; i<l; i++) {
				if (this.scache[i] == value) {
					r = i;
					break;
				}
			}
			if (r != null) {
				this.buf += "R";
				this.buf += r;
				return;
			}
			this.scache.push(value);
			this.buf += "y";
			value = encodeURIComponent(value);
			this.buf += value.length;
			this.buf += ":";
			this.buf += value;
		};

		public function toString(): String {
			return (this.buf.toString());
		};

		static public function run(value:*): String {
			var ser: Serializer = new Serializer()
			ser.serialize(value);
			return ser.toString();
		}
	}
}