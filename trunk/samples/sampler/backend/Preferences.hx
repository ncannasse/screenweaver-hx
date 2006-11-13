import systools.Registry;
class Preferences {

	public var winWidth: Int;
	public var winHeight: Int;
	public var cmdHaxe: String;
	public var cmdMTASC: String;
	public var cmdNeko: String;
	public var cmdBoot: String;
	public var __dev: Int;
	
	static var SUBKEY: String = "ScreenweaverHX.Sampler.Preferences";
	static var BOOT_MAC: String = "cp -R ../../tools/SWHX.app .;open swhx.app";
	static var BOOT_WIN: String = "start ..\\..\\tools\\SWHX.exe";
	static var BOOT_DEV: String = "start ..\\..\\bin\\win\\SWHX.exe";
	
	public function new() {
		load();		
	}
	
	public function save() {
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"winWidth",Std.string(winWidth));
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"winHeight",Std.string(winHeight));
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdHaxe",cmdHaxe);
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdMTASC",cmdMTASC);
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdNeko",cmdHaxe);
		Registry.setValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdBoot",cmdBoot);
	}
	
	public function load() {
		__dev = Std.parseInt(Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"__dev"));		
		winWidth = Std.parseInt(Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"winWidth"));
		if (winWidth==null || winWidth==0) winWidth=650;
		winHeight = Std.parseInt(Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"winHeight"));			
		if(winHeight==null || winHeight==0) winHeight=550;
		cmdHaxe = Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdHaxe");		
		if(cmdHaxe==null || cmdHaxe=="null") cmdHaxe="haxe";
		cmdMTASC = Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdMTASC");
		if(cmdMTASC==null || cmdMTASC=="null") cmdMTASC="mtasc";
		cmdNeko = Registry.getValue(Registry.HKEY_CURRENT_USER,SUBKEY,"cmdNeko");
		if(cmdNeko==null || cmdNeko=="null") cmdNeko="neko";
		if (neko.Sys.systemName()=="Mac")
			cmdBoot=BOOT_MAC;
		else 
			cmdBoot=BOOT_WIN;
		// for local development test only:
		if (__dev != null && __dev==1)
			cmdBoot = BOOT_DEV;
	}	
}