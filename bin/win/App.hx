class App {

	static var flash : swhx.Flash;
	static var wnd : swhx.Window;

	static function main() {
		swhx.Application.init();

		var width = 500;
		var height = 500;
				
		var server = new neko.net.RemotingServer();
		server.addObject("App",App);

		wnd = new swhx.Window
			( "Plain Window"
			, width,height
			// optional Window flags
			, 0
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
	
	static function foo(x,y) {
		return x + y;
	}

}
