
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
    	hook = window.addMessageHook(untyped 0x200 /*WM_MOUSEMOVE on Windows*/);
    	hook.setNekoCallback(mouseMoveHook);
    	/*
    	
    	Additionally, C to C hooks can be set. When set, the C handler takes
    	preference over the Neko handler. The handler in a .NDLL should be passed
    	as an abstract to a C function pointer. SWHX uses type 'k_window_msg_cb',
    	and expects the function type to be:
    	
    	'void *(*msg_hook_callback) ( window_msg_hook *h, void *id1, void *id2, void *p1, void *p2 )'
    	
    	For setting the handler via haXe use:
    	
    	hook.setCCallback(|function-ptr-abstrac-value|);
		*/
    }
    
    static function mouseMoveHook() {    	
		trace("Windows Mouse Move!" +"("+hook.p1+","+hook.p2+")");		
    	return 0;
    }    
}
