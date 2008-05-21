class App {

	static var flash : swhx.Flash;
	static var cnx: swhx.Connection;

	static function filesDropped( files: Array<String> ) : Bool{
		for( i in 0...files.length ){
			cnx.Flash.display.call(["Dropped file: "+files[i]]);
		}
		return true;
	}

	static function main() {
		// initialize SWHX
		swhx.Application.init();

		// create window, flash and load
		var wnd = new swhx.Window("Hello World !",400,300);
		flash = new swhx.Flash(wnd);
		flash.setAttribute("id","myclip");
		flash.setAttribute("src","ui.swf");
		flash.start();
		wnd.show(true);

		// set window to accept file drops
		// this can be toggled on and of at any time, using roll-overs/out
		// one can create 'drop-spots'
		wnd.dropTarget = true;
		// set event handler for file drops
		wnd.onFilesDropped = filesDropped;

		// connect to our front-end:
		cnx = swhx.Connection.flashConnect(flash);

		// enter application message loop:
		swhx.Application.loop();

		// clean up:
		swhx.Application.cleanup();
	}

}