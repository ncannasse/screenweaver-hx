class App {

	static var flash : swhx.Flash;

	static function hello( x : Int, y : Int ) {
		return "hello "+(x+y);
	}
	
	static function stringTest( string : String ) {
		return string;
	}

	static function main() {		
		swhx.Application.init(9); // Flash player 9 required.

		var width = 550;
		var height = 400;
		
		// server		
		var server = new neko.net.RemotingServer();
		server.addObject("backend",App);

		// window
		var wnd = new swhx.Window("Screenweaver HX AS3 API Sample",width,height);
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
		};		
		wnd.resizable = true;		
		wnd.show(true);		

		// flash;
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
		trace("Main Flash content loading complete.");
		var cnx = swhx.Connection.flashConnect(flash);
		trace("hello(4,5) = "+cnx.hello.call([4,5]));
	}
}
