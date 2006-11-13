class Flash {

	// compile swhx.Connection in order to be able
	// to get incoming communications
	static var enabledCom = swhx.Connection;
	
	static public function display( msg : String ) {
		trace(msg);
	}
	
	static function main() {
		trace("Start");
		trace("Please drop some files on this window...");		
	}
}