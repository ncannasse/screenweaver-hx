class Sample {

	public var title(getTitle,null): String;
	public var description(getDescription,null): String;
	public var path(default,null): String;
	public var file(default,null): String;
	public var buildCommands(getBuildCmds,null): Array<String>;
	public var runCommands(getRunCmds,null): Array<String>;
	public var build(default,default): Bool;
	
	private var xml: Xml;
	private var parsed: Bool;	

	public function new(path: String, file: String) {
		this.path = path;
		this.file = file;
		this.parsed = false;
		this.build = false;
	}
	
	public function parse() : Bool {		
		parsed = false;				
		var content = neko.io.File.getContent(file);		
		try {
			xml = Xml.parse(content);
			parsed = true;
		}
		catch(e: Dynamic) {		
		}		
		return parsed;
	}
	
	public function getTitle() : String {		
		try {			
			return xml.firstElement().get("title");
		} 
		catch (e: Dynamic) {
			return "Invalid XML: "+e;
		}
	}
	
	public function getDescription() : String {		
		try {
			var xmlfile = xml.firstElement().get("description");
			return neko.io.File.getContent(path+"/"+xmlfile);
		}
		catch (e: Dynamic) {
			return "Invalid XML: "+e;
		}
	}
	
	public function getBuildCmds(): Array<String> {		
		var node = xml.firstElement().elementsNamed("Build");
		return getCommands(node);
	}
	
	public function getRunCmds(): Array<String> {		
		var node = xml.firstElement().elementsNamed("Run");
		return getCommands(node);
	}
	
	static public function getCommands(node: Iterator<Xml>): Array<String> {
		var result = new Array();
		for (n in node) {
			var elements = n.elements();
			for(e in elements) {				
				var cmd;
				// select command:
				if (e.nodeName=="Haxe") {					
					cmd = Sampler.p.cmdHaxe;
				} else if (e.nodeName=="MTASC") {					
					cmd = Sampler.p.cmdMTASC;									
				} else if (e.nodeName=="Manual") {
					// cannot be build or run:
					return null;
				} else if (e.nodeName=="Neko") {					
					cmd = Sampler.p.cmdNeko+cmd;
				} else if (e.nodeName=="Boot") {					
					cmd = Sampler.p.cmdBoot;
				} else {
					cmd = e.nodeName;
				}
				// add arguments:
				if (e.firstChild()!=null)
					cmd += " "+e.firstChild().nodeValue;
				// store command:
				result.push(cmd);
			}
		}	
		return result;
	}	
}