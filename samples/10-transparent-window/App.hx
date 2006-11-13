class App {

	static var flash : swhx.Flash;
	static var wnd : swhx.Window;

	static function main() {
		swhx.Application.init();

		var width = 200;
		var height = 200;
				
		var server = new neko.net.RemotingServer();
		server.addObject("backend",App);

		wnd = new swhx.Window
			( "Transparent Window"
			, width,height
			// optional Window flags
			, swhx.Window.WF_TRANSPARENT
			);

		flash = new swhx.Flash(wnd,server);
		flash.setAttribute("id","ui");
		flash.setAttribute("src","ui.swf");
		flash.start();
		wnd.resizable = true;		
		wnd.show(true);		
				
		swhx.Application.loop();
		swhx.Application.cleanup();
	}
		
	static function doClose() {		
		wnd.destroy();
	}
}
