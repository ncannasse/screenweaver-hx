
class App {

	// create a file and save the content inside
	static function saveFileContent( file : String, content : String ) {
		// create the file in text mode
		var f = neko.io.File.write(file,true);
		// write the content
		f.writeString(content);
		// close the file
		f.close();
	}

    static function main() {
        // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window with title "My Application"
        var window = new swhx.Window("My Application",400,300);

		// create an incoming communication Server
		var context = new haxe.remoting.Context();

		// share the App object
		context.addObject("backend",App);

        // create a flash object inside this window
        // pass the server as parameter
        var flash = new swhx.Flash(window,context);

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
