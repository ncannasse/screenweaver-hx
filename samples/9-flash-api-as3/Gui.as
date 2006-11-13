package {
	import swhx.*
	import flash.display.*
	import flash.text.*
	
	public class Gui extends MovieClip{
		
		private static var me: Gui = null;		
		private var log: TextField;		
		public var gui: Sprite;
		
		public static function instance() : Gui {
			return me;
		}
		
		public function Gui() {
			trace("hello from gui");
			if (me == null) { 
				me = this;			
				init();
			}
		}
			
		private function init() {			
			// Initialize the Screenmeaver HX binding.
			// the 'me' argument specifies the base object swhx
			// will use on trying to resolve external calls from
			// the haXe back-end.		
			swhx.Api.init(me);				
			log = gui.getChildByName("log");			
			
			// call haxe:
			callHaxe();		
		}
		
		public function hello(x: Number, y: Number) : String {
			var result = "hello "+ (x + y);
			log.appendText( "haXe is calling function Hello("+x+","+y+") on Flash:\nFlash responds with: " );
			log.appendText( result + "\n" );
			return result;
		}
		
		public function callHaxe() {
			// Invoke a method on object 'obj' that lives in our haXe back-end,
			// and display the result:
			log.appendText( "Flash is calling function backend.hello(10,20) on haXe:\nhaXe responds with: " );
			log.appendText( "" + swhx.Api.call("backend.hello",10,20)  );
			log.appendText( "\n" );
		}		
	}
}