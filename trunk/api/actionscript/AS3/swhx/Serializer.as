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
		j : utf8 escaped string
		k : NaN
		l : list		(not supported)
		m : -Inf
		n : null
		o : object
		p : +Inf
		q : inthash		(not supported)
		r : reference
		s : utf8 string
		t : true
		u : array nulls
		v : date		(not supported)
		w : enum		(not supported)
		x : exception
		y : *unused		(not supported)
		z : zero		(not supported)
		*/

		private var buf: String;
		private var ocache: Array;
		private var scache: Array;

		static var reBackslash: RegExp = /\\/g;
		static var reNewline: RegExp = /\n/g;
		static var reReturn: RegExp = /\r/g;

		public function Serializer() {
			buf = "";
			ocache = new Array();
			scache = new Array();
		};

		public function dontUseCache() {
			this.ocache = null;
		};

		public function serialize(value) {
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
				var str = value.toString();
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
				var i = 0;
				var l = value.length;
				var c = 0;
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
			if (value instanceof Function) {
				throw "Flash serializer Cannot serialize function: '"+value+"'\n";
			}

			// Object
			if (value is Object) {
				this.buf += "o";
				for(var i in value) {
					this.serializeString(i);
					this.serialize(value[i]);
				}
				this.buf += "g";
				return;
			}

			// shouldn't reach this:
			throw "Flash deserializer Unknown value type";
		};

		public function serializeException(value) {
			this.buf += "x";
			this.serialize(value);
		};

		public function serializeRef(value): Boolean {
			if (this.ocache == null) {
				return (false);
			}
			var i = 0;
			var l = this.ocache.length;
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

		public function serializeString(value: String) {
			var r = null;
			var l = this.scache.length;
			for (var i=0; i<l; i++) {
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

		static public function run(value): String {
			var ser: Serializer = new Serializer()
			ser.serialize(value);
			return ser.toString();
		}
	}
}