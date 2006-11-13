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
			gui._x = 2;
			gui._y = 4;
			
			log = gui.log;
			gui.btnLogo.onPress 	= function () {	swhx.Api.call("backend.wnd.drag"); }
			gui.btnClose.onPress	= function () { swhx.Api.call("backend.wnd.destroy"); }
			gui.btnMin.onPress		= function () { swhx.Api.call("backend.doMinimize"); }
			gui.btnMax.onPress		= function () { swhx.Api.call("backend.doMaximize"); }
			gui.btnTL.onPress		= function () { swhx.Api.call("backend.wnd.resize","TL"); }
			gui.btnT.onPress		= function () { swhx.Api.call("backend.wnd.resize","T"); }
			gui.btnTR.onPress		= function () { swhx.Api.call("backend.wnd.resize","TR"); }
			gui.btnL.onPress		= function () { swhx.Api.call("backend.wnd.resize","L"); }
			gui.btnR.onPress		= function () { swhx.Api.call("backend.wnd.resize","R"); }
			gui.btnBL.onPress		= function () { swhx.Api.call("backend.wnd.resize","BL"); }
			gui.btnB.onPress		= function () { swhx.Api.call("backend.wnd.resize","B"); }
			gui.btnBR.onPress		= function () { swhx.Api.call("backend.wnd.resize","BR"); }
			
			// no resizing:
			Stage.scaleMode = "noScale";
		}		
	}
}