@echo off
haxe test.hxml && neko app
if errorlevel 1 goto error
goto end

:error
pause
:end

