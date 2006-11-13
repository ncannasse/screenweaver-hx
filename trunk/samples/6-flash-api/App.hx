class App {

	static var flash : swhx.Flash;
	static var cnx: swhx.Connection;
	static var wnd: swhx.Window;

	static function hello( x : Int, y : Int ) {
		return "hello "+(x+y);
	}

	static function main() {
		swhx.Application.init();

		var width = 550;
		var height = 400;
		
		// server		
		var server = new neko.net.RemotingServer();
		server.addObject("backend",App);

		// window
		wnd = new swhx.Window("Hello World !",width,height);		
		wnd.onMinimize = function () { 
			trace("window is minimizing"); 
			return true; // return false to block minimize;
		};		
		wnd.onMaximize = function () { 
			trace("window is maximizing"); 
			return true; // return false to block maximize;
		};		
		wnd.onRightClick = function() {
			trace("Supressing right-click menu by returning false on event");
			return false;
		}
		wnd.resizable = true;
		wnd.show(true);

		// flash
		flash = new swhx.Flash(wnd,server);
		flash.setAttribute("id","ui");
		flash.setAttribute("src","ui.swf");		
		flash.onSourceLoaded = onSourceLoaded;
		flash.start();
						
		// message loop:
		swhx.Application.loop();
		swhx.Application.cleanup();
	}
	
	static function onSourceLoaded() {		
		trace("on source loaded");
		// connection
		cnx = swhx.Connection.flashConnect(flash);
		// use swhx.Connection to call a method in the GUI:
		trace("hello(4,5) = "+cnx.hello.call([4,5]));
	}

}
