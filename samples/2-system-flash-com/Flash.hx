
class Flash {

	static function drawRectangle( x, y, w, h, c ) {
        // draw a red rectangular shape
        var mc = flash.Lib.current;
		#if flash9 var mc = mc.graphics; #end
        mc.moveTo(x,y);
        mc.beginFill(c,100);
        mc.lineTo(x+w,y);
        mc.lineTo(x+w,y+h);
        mc.lineTo(x,y+h);
        mc.endFill();
	}

    static function main() {
		swhx.Connection.desktopConnect();
		drawRectangle(10,10,100,100,0xFF0000);
    }
}
