class Command {

	public var line(default,null): String;
	public var result(default,null): Int;

	var cb: Command->Void;
	var progress: String->Void;

	public function new(line: String,progress: String->Void, ?cb: Command->Void) {
		this.line = line;
		this.cb = cb;
		this.progress = progress;
		this.result = 1;
		swhx.Task.queue(execute,onResult);
	}

	public function onResult(cmdres: Int): Void {
		result = cmdres;
		if (this.cb != null) this.cb(this);
	}

	public function execute(): Int {
		progress("<p>Please hold while command</p><p><b>"+this.line+"</b></p><p>is being executed</p>");
		return neko.Sys.command(line);
	}

}