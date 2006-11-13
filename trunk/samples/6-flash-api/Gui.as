import swhx.*

class Gui {
	
	private static var me: Gui = undefined;
	private static var gui: MovieClip;
	private static var log: TextField;
	
	public static function instance() : Gui {
		return me;
	}
	
	public function hello(x: Number, y: Number) : String {
		var result = "hello "+ (x + y);
		log.text += "haXe is calling function Hello("+x+","+y+") on Flash:\nFlash responds with: ";
		log.text += result + "\n"
		return result;
	}
	
	public static function callHaxe() {
		// Invoke a method on object 'obj' that lives in our haXe back-end,
		// and display the result:
		log.text += "Flash is calling function backend.hello(10,20) on haXe:\nhaXe responds with: ";
		log.text += swhx.Api.call("backend.hello",10,20);
		log.text += "\n";
	}	
	
	public static function main(root: MovieClip) {
		if (!me) {
			me = new Gui();			
			
			// Initialize the Screenmeaver HX binding.
			// the 'this' argument specifies the base object swhx
			// will use on trying to resolve external calls from
			// the haXe back-end.		
			swhx.Api.init(me);			
		
			// Setup our graphical representation:
			gui = root.attachMovie("GuiAssetsId","gui",1);
			gui._x = 10;
			gui._y = 5;
			
			log = gui.log;
						
			// call haxe:
			callHaxe();
		}		
	}
}