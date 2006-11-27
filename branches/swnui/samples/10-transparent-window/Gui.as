import swhx.*

class Gui {
	
	private static var me: Gui = undefined;
	private static var gui: MovieClip;
	private static var log: TextField;
	private static var btn: Button;
	
	public static function instance() : Gui {
		return me;
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
			gui._x = 0;
			gui._y = 0;
			
			log = gui.log;
			gui.mcLogo.onPress 	= function () { swhx.Api.call("backend.wnd.drag"); }
			gui.btnClose.onPress	= function () { swhx.Api.call("backend.wnd.destroy"); }
						
			// no resizing:
			Stage.scaleMode = "noScale";
		}		
	}
}