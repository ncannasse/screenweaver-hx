class Flash {

	static public function display( msg : String ) {
		trace(msg);
	}

	static function main() {
		swhx.Connection.desktopConnect();
		trace("Start");
		trace("Please drop some files on this window...");
	}
}