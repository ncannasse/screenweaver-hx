
class App {

	// handle the "fscommand" calls from Flash
	static function handleFSCommand( cmd : String, params : String ) {

		switch( cmd ) {
		// save a file on the local drive
		case "save_file":
			var p = params.split(":");
			var file = p.shift();
			var f = neko.io.File.write(file,false);
			f.write(p.join(":"));
			f.close();

		default:
			throw "Unknown fscommand"+cmd;
		}

		return null;
	}

    static function main() {
        // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window with title "My Application"
        var window = new swhx.Window("My Application",400,300);

        // create a flash object inside this window
        var flash = new swhx.Flash(window,null);

        // set the HTML attributes of this flash object
        flash.setAttribute("src","ui.swf");

        flash.onFSCommand = handleFSCommand;


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
