
class Flash {

    static function main() {
		// connect to the desktop
		var cnx = swhx.Connection.desktopConnect();
		var data = "Hello world !\nA \"quoted\" string and <some> &lt;html&gt;\r\n\t \\n \\\\r and special chars.";
		// save a string to the file "hello.txt"
		cnx.App.saveFileContent.call(["hello.txt",data]);
		trace("The hello.txt file has been created !");
    }
}
