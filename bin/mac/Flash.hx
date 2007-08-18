﻿class Flash {

	static var cnx: swhx.Connection;

	static public function print( msg : String ) {
		trace(msg);
	}

	static function main() {
		trace("Hello world");
		cnx = swhx.Connection.desktopConnect();
		trace( cnx.App.foo.call([1,2]) );
		var h = new haxe.Http("test.hxml");
		h.onError = print;
		h.onData = function(d:String) { trace(d.length); };
		for( i in 0...50 )
			h.request(false);
	}
}
