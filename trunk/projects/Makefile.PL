$SRC = "../src";
$DEST = "../bin/linux";
#../bin/linux";
$XLIBS="-lXt -lXmu -lXp -lXtst";
$I = "-I$SRC -I./../include -I$SRC/linux -I/usr/include";
$MOZ_GTK2_CFLAGS="`pkg-config --cflags gtk+-2.0`";
$MOZ_GTK2_LIBS = "`pkg-config --libs gtk+-2.0`";
$L = "-L/usr/lib -lneko -lz -ldl -D LINUX";

$folders = qw(api  bin  include  src);
$files = "$SRC/np_host.c $SRC/api.c $SRC/flash.c $SRC/flash_dll.c $SRC/linux/gtk2xtbin.c $SRC/linux/system.c";
system("gcc -g -fPIC -shared -o $DEST/swhx.ndll $I $files $MOZ_GTK2_CFLAGS $L $MOZ_GTK2_LIBS $XLIBS");
