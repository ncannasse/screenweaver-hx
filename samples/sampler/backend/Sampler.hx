import Types;

class Sampler {
	
	/*
	* Back-end containers
	*/
	static var app: swhx.Application;
	static var win: swhx.Window;	
	static var srv: neko.net.RemotingServer;
	static var ui: swhx.Flash;
	static var cnx: swhx.Connection;
	
	static var mac: Bool = neko.Sys.systemName()=="Mac";
	
	/*
	* Local storage
	*/
	static var s: Array<Sample>;
	static var xml: Xml;
	
	/* 
	* Public storage
	*/
	static public var p(default,null): Preferences;
	
	/*
	* Entry point
	*/
	static public function main() {		
		swhx.Application.init(9); // require plug-in version 9 or better;
						
		p =	new Preferences();
		// FlashDevelop doesn't launch us from the BIN folder,
		// correct this problem if preference '__def' is set in the 
		// registry / preferences:
		if (p.__dev!=0) 
			neko.Sys.setCwd("../bin");	
		
		s = new Array();
		win = new swhx.Window("Screenweaver HX Sampler",p.winWidth,p.winHeight);
		srv = new neko.net.RemotingServer();
		srv.addObject("sampler",Sampler);
		
		ui = new swhx.Flash(win,srv);				
		ui.setAttribute("id","ui");
		ui.setAttribute("src","sampler.swf");
		ui.onSourceLoaded = onSourceLoaded;
		ui.start();
		
		win.resizable = true;		
		win.show(true);	
		win.onClose = function () : Bool {
			p.winWidth = win.width;
			p.winHeight = win.height;
			// save prefs:
			p.save();
			return true;
		}	
		
		swhx.Application.loop();
		swhx.Application.cleanup();		
	}
	
	/**
	* Invoked by SWHX after prim. source file hase been loaded
	*/	
	static function onSourceLoaded() {
		cnx = swhx.Connection.flashConnect(ui);
	}
	
	/**
	* Invoked by UI after initialized
	*/
	static public function onUIReady() {	
		var path = "../../";		
		var folders = neko.FileSystem.readDirectory(path);
		folders = sortByPrefixedNumber(folders);
		for (i in 0...folders.length) {
			var xmlpath = path+folders[i];
			if (neko.FileSystem.isDirectory(xmlpath)) {
				var xmlfile = xmlpath+"/sample.xml";
				if (neko.FileSystem.exists(xmlfile)) {					
					var sample = new Sample(xmlpath,xmlfile);
					if (sample.parse()) {						
						s.push(sample);
						var entry: TListEntry = 
							{ label:sample.title
							, descr:sample.description
							};
						cnx.list.call([entry]);
					}					
				}
			}
		}		
	}
	
	/*
	* Tool function for tracing to UI console
	*/
	static public function print(msg: String) {
		cnx.print.call([msg]);		
	}
	
	/*
	* Tool function for making path's OS compliant:
	*/
	static public function ospath(path: String): String {
		return if (mac) path; else path.split("/").join("\\");
	}
	
	/* 
	* Tool function for sorting folder list
	*/
	static public function sortByPrefixedNumber(a: Array<String>): Array<String> {
		a.sort(
			function(s1: String, s2: String): Int {
				var i1 = getIntPrefix(s1);
				var i2 = getIntPrefix(s2);
				if (i1 == i2)
					return 0;
				else if (i1 > i2)
					return 1;
				else
					return -1;
			}
		);
		return a;
	}
	
	/* 
	* Helper function to sorting folder list
	*/
	static public function getIntPrefix(s: String): Int {
		var p : String = "0";		
		for (i in 0...s.length) {
			var c: Int = s.charCodeAt(i);
			if (c >47 && 58>c)
				p += s.charAt(i);
			else 
				break;
		}
		return Std.parseInt(p);
	}
	
	/*
	* Open source folder to sample
	*/
	static public function sources(index: Int): Int {
		if (index<s.length) {
			var cmd = if (mac) "open "; else "explorer ";				
			cmd += s[index].path;
			neko.Sys.command(ospath(cmd));
		}
		return 0;
	}
	
	/*
	* Build and run sample
	*/
	static public function run(index: Int): Int {
		if (index<s.length) {
			cnx.busy.call([]);
			var sample = s[index];
			if (sample.buildCommands==null && !sample.build) {
				print("\nThis sample requires manual build steps before it can run properly. Please refer to the source files for further instruction.\nThe folder containing the samples sources will now be openened. Issue 'Run' once more after the manual build steps have been completed.");
				sources(index);
				sample.build = true;
				cnx.done.call([]);
				return 0;
			}
			var olddir = neko.Sys.getCwd();			
			neko.Sys.setCwd(ospath(sample.path));
			trace("setting path: "+ospath(sample.path));			
			if (!sample.build) {
				for (bc in sample.buildCommands) {
					print("Queueing build command:\n"+bc);
					new Command(bc,progress,workerCommand_callback);
				}
				sample.build = true;
			}			
			for (rc in sample.runCommands) {				
				print("Queueing run command:\n"+rc);
				new Command(rc,progress,workerCommand_callback);
			}
			trace("current path: "+ospath(sample.path));
			var f = callback(neko.Sys.setCwd,olddir);
			swhx.Task.queue(f,run_callback);				
		}
		return 0;
	}	
			
	/*
	* Callbacks
	*/	
	static function workerCommand_callback(cmd: Command) {
		var strres = if (cmd.result == 0) 
				"successful"; 
			else 
				"failure #"+cmd.result+". Please refer to the manual build instructions. Perhaps not all required software is installed. Current working dir: "+neko.Sys.getCwd();
			
		print("Command:\n"+cmd.line+"\nreturned "+strres);				
	}
		
	static function run_callback(Void) : Void {
		print("Done building and launching sample.");
		cnx.done.call([]);			
	}
	
	static function progress(msg: String) : Void {
		cnx.busyUpdate.call([msg]);
	}
	
	static var sync_call = neko.Lib.load("swhx","sync_call",1);
}