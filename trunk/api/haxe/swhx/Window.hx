/*
 * Copyright (c) 2007, Edwin van Rijkom, Nicolas Cannasse
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE HAXE PROJECT CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE HAXE PROJECT CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
package swhx;

/**
An SWHX application consists of one or more windows.
By instantiating a Window class, a new application window gets created.
Use the available meth
**/
class Window {

	var w : Void;
	
	/**Get/Set allowing operating system default chrome window resizing.**/
	public var resizable(getResizable,setResizable) : Bool;
	
	/**Get/Set operating system default chrome maximize button presence/enablement.**/
	public var maximizeIcon(getMaxIcon,setMaxIcon) : Bool;
	
	/**Get/Set operating system default chrome minimize button presence/enablement.**/
	public var minimizeIcon(getMinIcon,setMinIcon) : Bool;
	
	/**Get/Set the width of the window's client area.**/
	public var width(getWidth,setWidth) : Int;
	
	/**Get/Set the height of the window's client area.**/
	public var height(getHeight,setHeight) : Int;
	
	/**Get/Set the window's vertical position on the desktop.**/
	public var left(getLeft,setLeft) : Int;

	/**Get/Set the window's horizontal position on the desktop.**/
	public var top(getTop,setTop) : Int;
	
	public var handle(getHandle,null) : Void;
	
	/**	
	Get/Set window full-screen mode.
	On OS-X, full-screen mode will disable the menu bar, task-switching and force quit.
	**/
	public var fullscreen(getFullScreen,setFullScreen) : Bool;
	
	/**Not implemented.**/	
	public var transparent(getTransparent,null) : Bool;
	
	/**''true'' if the window contains a running Flash instance.**/	
	public var flashRunning(getFlashRunning,null) : Bool;	
	
	/**Get/Set window file-drop acceptance.**/
	public var dropTarget(getDropTarget,setDropTarget) : Bool;
	
	/**Get/Set window title.**/
	public var title(default,setTitle) : String;
	
	/**''true'' if the window was created with the WF_PLAIN flag set.**/
	public var plain(getPlain,null) : Bool;
	
	/**Get/Set window minimized state.**/
	public var minimized(getMinimized,setMinimized) : Bool;
	
	/**Get/Set window maximized state.**/
	public var maximized(getMaximized,setMaximized) : Bool;
	
	public static var WF_FULLSCREEN	= 1;
	public static var WF_TRANSPARENT	= 1 << 1;
	public static var WF_DROPTARGET	= 1 << 3;
	public static var WF_PLAIN			= 1 << 4;
	public static var WF_ALWAYS_ONTOP	= 1 << 5;
	public static var WF_NO_TASKBAR	= 1 << 6;

	public function new( title : String, width : Int, height : Int, ?flags : Int ) {		
		this.title = title;
		if (flags == null) flags = 0;
		w = _window_create(untyped title.__s,width,height,flags);		
		// late binding of events
		var me = this;
		_window_on_destroy(w,function() { me.onDestroy(); });
		_window_on_close(w,function() { return me.onClose(); });
		_window_on_minimize(w,function() { return me.onMinimize(); });
		_window_on_maximize(w,function() { return me.onMaximize(); });
		_window_on_rightclick(w,function() { return me.onRightClick(); });
		_window_on_filesdropped(w,function(s) {
				var a : Array<String> = untyped Array.new1(s,__dollar__asize(s));
				for( i in 0...a.length ) a[i] = new String(a[i]);
				return me.onFilesDropped(a);
			});
		_window_on_restore(w,function() { return me.onRestore(); });	
	}

	/**Show or hide the window.**/
	public function show( b : Bool ) {
		_window_show(w,b);
	}

	/**Destroy the window.**/
	public function destroy() {
		_window_destroy(w);
	}
	
	public function addMessageHook(msgid1: Void, ?msgid2: Void) {		
		return new MessageHook
			(	_window_add_message_hook(w,msgid1,if (msgid2!=null) msgid2 else untyped 0)
			,	msgid1
			,	if (msgid2!=null) msgid2 else untyped 0
			);
	}
	
	public function removeMessageHook(h: MessageHook) {
		_window_remove_message_hook(w,h);
	}
		
	/**
	<p>
	Initiate user window resizing.  
	<p>
	Specify either "L","R","T","TL","TR","B","BL" or "BR" to set the resize direction.
	Windows on OSX ignore this argument and always resizes from the bottom-right ("BR") corner of the window.
	**/
	public function resize(?direction: String) {
		var i;		
		switch (direction) {
			case "L": i=1;
			case "R": i=2;
			case "T": i=3;
			case "TL": i=4;			
			case "TR": i=5;
			case "B": i=6;			
			case "BL": i=7;			
			case "BR": i=8;
			default: i=8;
		}
		_window_resize(w,i);		
	}
	
	/**Initiate user window dragging.**/
	function drag() {
		return _window_drag(w);
	}

	/**Event invoked on window destruction.**/
	public function onDestroy() {
		Application.exitLoop();
	}

	/**Event invoked on window closure. Returning ''false'' from the event handler will cancel window closure.**/
	public function onClose() {
		return true;
	}

	/**
	Event invoked when the window is minimized from the operating system window chrome. 
	Returning ''false'' from the event handler will minimization.
	**/
	public function onMinimize() {
		return true;
	}

	/**
	Event invoked when the window is maximized from the operating system window chrome.
	Returning ''false'' from the event handler will maximization.
	**/
	public function onMaximize() {
		return true;
	}

	/**
	Event invoked when the user right-clicks in the window's client area.
	Returning ''false'' from the event handler will prevent the event being forwarded to the Flash player.
	**/
	public function onRightClick() {
		return true;
	}

	/**Event invoked when the user drops files on the window's client area while Window.dropTarget is enabled.**/
	public function onFilesDropped( files : Array<String> ) {
		return true;
	}
	
	/**Event invoked when the window is restored by the user.**/
	public function onRestore() {	
	}

	function getResizable() {
		return _window_get_prop(w,0) != 0;
	}

	function setResizable( b ) {
		_window_set_prop(w,0,if( b ) 1 else 0);
		return b;
	}

	function getMaxIcon() {
		return _window_get_prop(w,1) != 0;
	}

	function setMaxIcon(b) {
		_window_set_prop(w,1,if( b ) 1 else 0);
		return b;
	}

	function getMinIcon() {
		return _window_get_prop(w,2) != 0;
	}

	function setMinIcon(b) {
		_window_set_prop(w,2,if( b ) 1 else 0);
		return b;
	}

	function getWidth() {
		return _window_get_prop(w,3);
	}

	function getHeight() {
		return _window_get_prop(w,4);
	}

	function getLeft() {
		return _window_get_prop(w,5);
	}

	function getTop() {
		return _window_get_prop(w,6);
	}

	function setWidth(i) {
		_window_set_prop(w,3,i);
		return i;
	}

	function setHeight(i) {
		_window_set_prop(w,4,i);
		return i;
	}

	function setLeft(i) {
		_window_set_prop(w,5,i);
		return i;
	}

	function setTop(i) {
		_window_set_prop(w,6,i);
		return i;
	}

	function getFullScreen() {
		return _window_get_prop(w,7) != 0;
	}

	function setFullScreen( b ) {
		_window_set_prop(w,7,if( b ) 1 else 0);
		return b;
	}

	function getTransparent() {
		return _window_get_prop(w,8) != 0;
	}

	
	function getFlashRunning() {
		return _window_get_prop(w,9) != 0;
	}
	
	function getDropTarget() {
		return _window_get_prop(w,10) != 0;
	}

	function setDropTarget( b ) {
		_window_set_prop(w,10,if( b ) 1 else 0);
		return b;
	}
	
	function setTitle(t: String) : String {		
		if (w!=null) _window_set_title(w,untyped t.__s);
		return t;
	}
	
	function getPlain() {
		return _window_get_prop(w,11) != 0;
	}
		
	function getMinimized() {
		return _window_get_prop(w,12) != 0;
	}
	
	function setMinimized( b ) {
		_window_set_prop(w,12,if( b ) 1 else 0);
		return b;
	}
	
	function getMaximized() {
		return _window_get_prop(w,13) != 0;
	}
	
	function setMaximized( b ) {
		_window_set_prop(w,13,if( b ) 1 else 0);
		return b;
	}
	
	function getHandle() {
		return _window_get_handle(w);
	}
	
	static var _window_create = neko.Lib.load("swhx","window_create",4);
	static var _window_show = neko.Lib.load("swhx","window_show",2);
	static var _window_destroy = neko.Lib.load("swhx","window_destroy",1);
	static var _window_set_prop = neko.Lib.load("swhx","window_set_prop",3);
	static var _window_get_prop = neko.Lib.load("swhx","window_get_prop",2);	
	static var _window_set_title = neko.Lib.load("swhx","window_set_title",2);
	static var _window_drag = neko.Lib.load("swhx","window_drag",1);
	static var _window_resize = neko.Lib.load("swhx","window_resize",2);
	static var _window_get_handle = neko.Lib.load("swhx", "window_get_handle", 1);
	static var _window_add_message_hook = neko.Lib.load("swhx", "window_add_message_hook", 3);
	static var _window_remove_message_hook = neko.Lib.load("swhx", "window_remove_message_hook", 2);
	
	static var _window_on_destroy = neko.Lib.load("swhx","window_on_destroy",2);
	static var _window_on_close = neko.Lib.load("swhx","window_on_close",2);
	static var _window_on_rightclick = neko.Lib.load("swhx","window_on_rightclick",2);
	static var _window_on_restore = neko.Lib.load("swhx","window_on_restore",2);
	static var _window_on_filesdropped = neko.Lib.load("swhx","window_on_filesdropped",2);
	static var _window_on_minimize = neko.Lib.load("swhx","window_on_minimize",2);
	static var _window_on_maximize = neko.Lib.load("swhx","window_on_maximize",2);	
}
