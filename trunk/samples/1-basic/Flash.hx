
class Flash {
    static function main() {
        // draw a red rectangular shape
        var mc = flash.Lib.current;
        mc.moveTo(10,10);
        mc.beginFill(0xFF0000,100);
        mc.lineTo(100,10);
        mc.lineTo(100,100);
        mc.lineTo(10,100);
        mc.endFill();
    }
}
