
class App {
    static function main() {
		/*
			Application.init's argument is a string containing the
			path to the Flash player plugin SWHX should use to
			run Flash content. Incase the value is null, SWHX will:
			try to (in this order, untill an option succeeds):
			1) 	look for a file named 'flashplayer.bin' in the
			working directory, that contains the Flash
			player

			2)	look for a pre-installed Flash player in the
			default browser plug-in folders.

			3)	download the latest Flash player from the
			internet and place it in the current working
			directory, under the name 'flashplayer.bin'
		*/

		// initialize ScreenWeaver HX
		swhx.Application.init();

		// create a 400x300 window with title "My Application"
		var window = new swhx.Window("My Application",40,30);

		// create a flash object inside this window
		var flash = new swhx.Flash(window);

		// set the HTML attributes of this flash object
		flash.setAttribute("src","ui.swf");

		// activate the Flash object
		flash.start();

		// display the window
		window.show(true);

		// disable resizing:
		window.resizable = false;

		// change width:
		window.width = 800;

		// change height:
		window.height = 600;

		// enter the system event loop (will exit when window is closed)
		swhx.Application.loop();

		// cleanup SWHX properly
		swhx.Application.cleanup();
    }
}
