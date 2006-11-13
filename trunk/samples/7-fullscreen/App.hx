class App {

	static var wnd : swhx.Window;

	static function main() {
		// initialize SWHX
		swhx.Application.init();

		// setup remoting server
		var server = new neko.net.RemotingServer();		

		// create window, flash and load
		wnd = new swhx.Window("Hello World !",400,300);		
		var flash = new swhx.Flash(wnd,server);
		flash.setAttribute("id","myclip");
		flash.setAttribute("src","ui.swf");
		flash.start();
		wnd.show(true);
				
		wnd.onRightClick = function () {
			trace("onRightClick!");
			// toggle full screen mode:
			wnd.fullscreen = !wnd.fullscreen;
			// don't forward this event to Flash:
			return false;
		}
				
		// enter application message loop:
		swhx.Application.loop();

		// clean up:
		swhx.Application.cleanup();
	}

}