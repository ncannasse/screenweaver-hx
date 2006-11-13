
class Flash {

	// compile swhx.Connection in order to be able
	// to get incoming communications
	static var enabledCom = swhx.Connection;

	static function drawRectangle( x, y, w, h, c ) {
        // draw a red rectangular shape
        var mc = flash.Lib.current;
        mc.moveTo(x,y);
        mc.beginFill(c,100);
        mc.lineTo(x+w,y);
        mc.lineTo(x+w,y+h);
        mc.lineTo(x,y+h);
        mc.endFill();
	}

    static function main() {
		drawRectangle(10,10,100,100,0xFF0000);
    }
}
