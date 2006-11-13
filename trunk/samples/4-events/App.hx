
class App {

	// a variable storing a method
	static var defaultUrlHandler : swhx.Stream -> String -> String -> Void = null;

	static function onCloseHandler() {
		// confirm if the window is closed
		return systools.Dialogs.confirm("Exit","Do you want to exit ?",false);
	}

	static function onUrlRequest( stream : swhx.Stream, url : String, postData : String ) {
		// open a dialogbox, only process the url request
		if( systools.Dialogs.confirm("Access","Do you allow access to "+url,true) ) {
			defaultUrlHandler(stream,url,postData);
			return;
		}
		// if the user click no, send some dummy content
		if( systools.Dialogs.confirm("Access","Do you want to send some dummy content instead ?",true) ) {
			stream.write("some dummy content");
			stream.close();
			return;
		}
		// will trigger an error on the Flash side
		stream.reportError();
	}

    static function main() {
        // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window with title "My Application"
        var window = new swhx.Window("My Application",400,300);

        // set the onClose event
        window.onClose = onCloseHandler;

        // create a flash object inside this window
        var flash = new swhx.Flash(window,null);

		// save the default URL handler
		defaultUrlHandler = flash.onGetURL;

		// change the getURL handler
		flash.onGetURL = onUrlRequest;

        // set the HTML attributes of this flash object
        flash.setAttribute("src","ui.swf");

        // activate the Flash object
        flash.start();

        // display the window
        window.show(true);

        // enter the system event loop (will exit when window is closed)
        swhx.Application.loop();

        // cleanup SWHX properly
        swhx.Application.cleanup();
    }
}
