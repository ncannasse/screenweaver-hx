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

import flash.external.ExternalInterface;

class swhx.Api {
	private static var me = null;
	private static var base: Object;

	private function Api(base: Object) {
		Api.base = base;
		ExternalInterface.addCallback("swhxCall",null,doCall);
		if( ExternalInterface.call(":connect","") != "ok" )
			trace("This SWF requires Screenweaver HX to run properly");
	}

	static private function doCall(funpath: String, argstr: String) {
		var fun: Object = resolvePath(funpath, base);
		if (fun) {
			var args = swhx.Deserializer.run(argstr);
			return swhx.Serializer.run(fun.fun.apply(fun.obj, args));
		} else {
			return "x"+swhx.Serializer.run(funpath+": Failed to resolve.");
		}
	}

	static public function resolvePath(path: String, obj: Object): Object {
		with (obj) {
			return { fun: eval(path), obj: obj};
		}
	}

	static public function init(base: Object): Api {
		if (me)
			return me;
		else
			return me = new Api(base);
	}

	static public function call() {
		var path = arguments.shift();
		var args = escapeString(swhx.Serializer.run(arguments));
		var result = ExternalInterface.call(path,args);
		return swhx.Deserializer.run(result);
	}

	static private function escapeString(str: String): String {
		return str.split("\\").join("\\\\").split("&").join("&amp;");
	}
}
