
class Flash {

    static function main() {
        // retrieve the content of "http://www.google.com"
		var h = new haxe.Http("http://www.google.com");
		// if an error occur, display it
		h.onError = function(e) {
			trace("ERROR = "+e);
		}
		// when html data received, display it
		h.onData = function(data) {
			trace(data);
		}
		// execute the request in GET
		h.request(false);
    }
}
