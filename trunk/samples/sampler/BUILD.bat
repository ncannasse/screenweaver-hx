@echo off
echo Compiling backend...
cd backend 
haxe sampler.hxml
cd ..
echo Compiling frontend...
cd frontend
"c:\Program Files\Macromedia\Flex2\bin\mxmlc" -compiler.source-path=../../../api/actionscript/AS3 -output ../bin/sampler.swf sampler.mxml
cd ..