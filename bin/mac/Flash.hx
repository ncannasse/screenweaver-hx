class Flash {

	static var cnx: swhx.Connection;

	static public function print( msg : String ) {
		trace(msg);
	}

	static function main() {
		cnx = swhx.Connection.desktopConnect();
		trace( cnx.App.foo.call([1,2]) );

		var h = new haxe.Http("test.hxml");
		h.onError = print;
		h.onData = function(d:String) { trace(d.length); };
		for( i in 0...150 )
			h.request(false);
	}
}
