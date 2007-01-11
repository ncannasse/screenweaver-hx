/*
 * Copyright (c) 2006, Edwin van Rijkom, Nicolas Cannasse
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

class Task {

	static var t = neko.vm.Thread.create(loop);

	public static function onError( e : Dynamic ) {
		var str = try Std.string(e) catch( e : Dynamic ) "???";
		neko.Lib.print(e + "\n" + haxe.Stack.toString(haxe.Stack.exceptionStack()));
	}

	static function loop() {
		while( true ) {
			var m = neko.vm.Thread.readMessage(true);
			try {
				var r = m.call();
				if( m.onResult != null )
					sync_call(function() { m.onResult(r); });
			} catch( e : Dynamic ) {
				onError(e);
			}
		}
	}

	public static function queue<T>( call : Void -> T, ?onResult : T -> Void ) {
		t.sendMessage({ call : call, onResult : onResult });
	}

	public static function async<T>( call : Void -> T, ?onResult : T -> Void ) {
		var f = function() {
			try {
				var r = call();
				if( onResult != null )
					sync_call(function() { onResult(r); });
			} catch( e : Dynamic ) {
				onError(e);
			}
		};
		neko.vm.Thread.create(f);
	}

	static var sync_call = neko.Lib.load("swhx","sync_call",1);
}
