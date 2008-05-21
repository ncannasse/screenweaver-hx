class App {

	static var flash : swhx.Flash;
	static var wnd : swhx.Window;

	static function main() {
		swhx.Application.init();

		var width = 438;
		var height = 460;

		var context = new haxe.remoting.Context();
		context.addObject("backend",App,true);

		wnd = new swhx.Window
			( "Plain Window"
			, width,height
			// optional Window flags
			, swhx.Window.WF_PLAIN
			);

		flash = new swhx.Flash(wnd,context);
		flash.setAttribute("id","ui");
		flash.setAttribute("src","ui.swf");
		flash.start();
		wnd.resizable = true;
		wnd.show(true);

		swhx.Application.loop();
		swhx.Application.cleanup();
	}

	static function doMinimize() {
		wnd.minimized = true;
	}

	static function doMaximize() {
		wnd.maximized = !wnd.maximized;
	}

	static function doClose() {
		trace("Destroying window");
		wnd.destroy();
	}
}
