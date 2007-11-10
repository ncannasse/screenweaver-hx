class Run {

	static var windows = neko.Sys.systemName()=="Windows";

	public static function main() {
		var cmd;
		var dir = neko.Sys.getCwd();
		neko.Sys.setCwd(ospath("samples/sampler/backend"));
		cmd = "haxe -main Sampler -neko ../bin/app.n -lib swhx";
		trace("buidling Sampler application - please hold.");
		neko.Sys.command(cmd);
		neko.Sys.setCwd(dir);
		var sampler = ospath("samples/sampler/bin");
		neko.Sys.setCwd(sampler);
		if (windows)
			cmd = "start ..\\..\\..\\tools\\SWHX.exe -swroot ";
		else {
			neko.Sys.command("chmod -R 755 ../../../tools/SWHX.app");
			// Uncomment to reset preferences:
			// neko.Sys.command("rm ~/Library/Preferences/ScreenweaverHX.Sampler.Preferences.plist");
			cmd = "../../../tools/SWHX.app/Contents/MacOS/swhx -swroot ";
		}
		cmd += '"'+dir+sampler+'"';
		trace("running Sampler application.");
		neko.Sys.command(cmd);
		trace("done");
	}

	public static function ospath(path: String): String {
		if (windows)
			return path.split("/").join("\\");
		else
			return path;
	}
}