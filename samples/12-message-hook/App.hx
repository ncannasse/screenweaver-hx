import systools.win.Events;

class App {

	static var flash : swhx.Flash;
	static var cnx: swhx.Connection;
	static var window: swhx.Window;
	static var hook: swhx.MessageHook;

    static function main() {
    	 // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window
        window = new swhx.Window("Window Message Hooks",400,300);

		window.onRightClick = function() {
			return false;
		}

		// create an incoming communication Server
		var context = new haxe.remoting.Context();

		// share the App object
		context.addObject("backend",App);

        // create a flash object inside this window
        // pass the context as parameter
        flash = new swhx.Flash(window,context);

        // set the HTML attributes of this flash object
        flash.setAttribute("src","ui.swf");

        // capture loaded event:
        flash.onConnected = onConnected;

        // activate the Flash object
        flash.start();

        // enter the system event loop (will exit when window is closed)
        swhx.Application.loop();

        // cleanup SWHX properly
        swhx.Application.cleanup();
    }

    static function onConnected() {
    	cnx = swhx.Connection.flashConnect(flash);
    	window.show(true);
    	trace("Window handle is:"+window.handle+" (will be readable when passed to an .ndll)");

    	hook = window.addMessageHook(untyped Events.RBUTTONUP /*WM_RBUTTONUP on Windows*/);
		hook.setNekoCallback(mouseRightClickHook);
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

	static function mouseRightClickHook() {
		trace( "Right mouse click detected! (P1:"+hook.p1+", P2: "+hook.p2+")" );
		// return message 'not-handled', so SWHX will continue
		// processing it:
		return 0;
	}
}
