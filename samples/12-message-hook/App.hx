import systools.win.Tray;
import systools.win.Menus;
import systools.win.Events;

class App {
	
	static var flash : swhx.Flash;
	static var cnx: swhx.Connection;
	static var window: swhx.Window;
	static var hook: swhx.MessageHook;
	static var thook: swhx.MessageHook;
	static var tray: Tray;
	static var m: Menus;

    static function main() {
    	 // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window
        window = new swhx.Window("Window Message Hooks",400,300);
		
		window.onRightClick = function() {
			return false;
		}

		// create an incoming communication Server
		var server = new neko.net.RemotingServer();

		// share the App object
		server.addObject("App",App);

        // create a flash object inside this window
        // pass the server as parameter
        flash = new swhx.Flash(window,server);

        // set the HTML attributes of this flash object
        flash.setAttribute("src","ui.swf");
        
        // capture loaded event:
        flash.onSourceLoaded = onSourceLoaded;

        // activate the Flash object
        flash.start();

        // enter the system event loop (will exit when window is closed)
        swhx.Application.loop();

        // cleanup SWHX properly
        swhx.Application.cleanup();    	
    }
    
    static function onSourceLoaded() {
    	cnx = swhx.Connection.flashConnect(flash);
    	window.show(true);
    	trace("Window handle is:"+window.handle+" (will be readable when passed to an .ndll)");
		
		m = new systools.win.Menus();
		m.addItem( "option 1", 1 );
		m.addItem( "option 2", 2 );
		m.addItem( "option 3", 3 );
		
    	hook = window.addMessageHook(untyped Events.RBUTTONUP /*WM_RBUTTONUP on Windows*/);
		hook.setNekoCallback(mouseRightClickHook);
    	thook = window.addMessageHook(untyped Events.TRAYEVENT);
		thook.setNekoCallback(mouseRightClickHook);
    	/*
    	
    	Additionally, C to C hooks can be set. When set, the C handler takes
    	preference over the Neko handler. The handler in a .NDLL should be passed
    	as an abstract to a C function pointer. SWHX uses type 'k_window_msg_cb',
    	and expects the function type to be:
    	
    	'void *(*msg_hook_callback) ( callbackData *void, void *id1, void *id2, void *p1, void *p2 )'
    	
    	For setting the handler via haXe use:
    	
    	hook.setCCallback(|function-ptr-abstract-value|);
		*/		
    }
    
    static function mouseMoveHook() {    	
		trace("Windows Mouse Move!" +"("+hook.p1+","+hook.p2+","+hook.callbackData+")");
    	return 0;
    }    
	
	static function mouseRightClickHook() {
		if ( Std.string(thook.p2) == Std.string(Events.RBUTTONUP) )
			trace( "Option " + m.show( window.handle ) + " was selected!" );
		return 0;
	}
}
