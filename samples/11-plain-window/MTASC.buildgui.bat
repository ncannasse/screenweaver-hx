mtasc -cp ..\..\api\actionscript\AS2 -main -version 8 -swf uiAssets.swf -out ui.swf Gui
haxe compile.hxml
copy ui.swf ..\..\bin
copy app.n ..\..\bin