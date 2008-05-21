class App {

	static var window: swhx.Window;
	static var flash: swhx.Flash;

    static function main() {
        // initialize ScreenWeaver HX
        swhx.Application.init();

        // create a 400x300 window with title "My Application"
		window = new swhx.Window("My Application",400,300);

        // create a flash object inside this window
        flash = new swhx.Flash(window);

        // set the HTML attributes of this flash object
        flash.setAttribute("src","ui.swf");

		// set the callback to be invoked on movie load completion
		flash.onConnected = onConnected;

        // activate the Flash object
        flash.start();

		// enter the system event loop (will exit when window is closed)
        swhx.Application.loop();

        // cleanup SWHX properly
        swhx.Application.cleanup();
	}

	static function onConnected() {
		// display the window
        window.show(true);

		// create a connection to the flash Object :
		var cnx = swhx.Connection.flashConnect(flash);

		// draw rectangles
		for( i in 0...100 ) {
			var x = Std.random(400);
			var y = Std.random(300);
			var w = Std.random(100);
			var h = Std.random(100);
			var c = Std.random(0x1000000);
			cnx.Flash.drawRectangle.call([x,y,w,h,c]);
		}
    }
}
