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

	import flash.external.ExternalInterface;

	public class Api {
		private static var me:Api = null;
		private static var base: Object;
		private static var desktop: Boolean;
		private static var reBackslash: RegExp = /\\/g;

		public function Api(base: Object) {
			Api.base = base;
			if (ExternalInterface.available) {
				ExternalInterface.addCallback("swhxCall",doCall);
				ExternalInterface.call(":init","");
			}
		}

		static public function get connected(): Boolean {
			return desktop;
		}

		static private function doCall(funpath: String, argstr: String) : String {
			var err:String = base.toString();
			try {
				var fun: Object = resolvePath(funpath, base);
				err+=fun+"|";
				if (fun) {
					var args:* = Deserializer.run(argstr);
					err+=args+"|";
					return Serializer.run(fun.fun.apply(fun.obj, args));
				} else {
					return "x"+Serializer.run("function '"+funpath+"' failed to resolve");
				}
			}
			catch(e:*) {
				err = 	"Api exception on invoking:\n "
						+funpath+":\nwith arguments:\n"
						+argstr+"\nerror path:\n"
						+err+"\nexception:\n"
						+e.toString();
				return "x"+Serializer.run(err);
			}
			// Flex 2 AS compiler isn't aware of the fact that this functon
			// will always return a value. Fix:
			return "";
		}

		static public function resolvePath(path: String, obj: Object): Object {
			var steps:Array = path.split(".");
			var fun:Object = obj;
			for (var i:int=0; i<steps.length; i++) {
				obj = fun;
				if (obj.hasOwnProperty(steps[i]))
					fun = fun[steps[i]];
				else
					return null;
			}
			return {fun: fun, obj: obj};
		}

		static public function init(base: Object): Api {
			desktop = false;
			if (ExternalInterface.available)
				if (ExternalInterface.call(":desktop",":available") == "yes")
					desktop = true;

			if (!desktop) trace("This SWF requires Screenweaver HX to run properly");

			if (me)
				return me;
			else
				return me = new Api(base);
		}

		static public function call(path:String,... args) : Object {
			if (desktop) try {
				var esc: String = escapeString(Serializer.run(args));
				return Deserializer.run(ExternalInterface.call(path,esc));
			} catch(e:*) {
				throw("Api exception:\n"+e+"\n"+path+"\n"+esc);
			}
			return null;
		}

		static private function escapeString(str: String): String {
			return str.replace(reBackslash,"\\\\");
		}
	}
}