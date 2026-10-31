/*
 * FreeBASIC graphics: js/gfx_mobile.c
 * Purpose: Report browser geometry and bridge canvas text entry to the IME.
 * Responsibilities: Live viewport metrics, bounded input regions and focus.
 * This file does not perform widget hit testing or render HTML application UI.
 * Threading: The browser main thread owns all DOM state and callbacks.
 */
#include "../../fb_gfx.h"
#include "../mobile_control.h"
#include <emscripten.h>

EM_JS(void, js_mobile_control, (int what, int a, int b, int c, int d), {
	if (typeof window === 'undefined' || typeof document === 'undefined') {
		if (what < 100) HEAP32.set([0, 0, 0, 0], a >> 2);
		return;
	}
	var state = Module['fbSoftKeyboard'];
	if (!state) {
		state = { requested: false, regions: [], input: null,
			available: navigator.maxTouchPoints > 0 || (window.matchMedia && window.matchMedia('(any-pointer: coarse)').matches) };
		Module['fbSoftKeyboard'] = state;
		state.occluded = function() {
			var visible = window.visualViewport && window.innerHeight - window.visualViewport.height > 100;
			if (navigator.virtualKeyboard && navigator.virtualKeyboard.boundingRect)
				visible = visible || navigator.virtualKeyboard.boundingRect.height > 0;
			return !!visible;
		};
		state.focus = function(visible, refocus) {
			state.requested = visible;
			if (!state.available) return;
			if (!visible) {
				if (state.input && document.activeElement === state.input) {
					state.input.blur();
					if (Module['canvas']) Module['canvas'].focus({ preventScroll: true });
				}
				return;
			}
			/* BASIC polls outside the browser's trusted input callback. Keep
			 * its show request pending until the canvas gesture can focus the
			 * editor without interrupting the press/release sequence. */
			if (!refocus) return;
			/* Firefox Android can retain a dismissed editor's native IME
			 * connection across blur/focus. A trusted retap needs a fresh DOM
			 * editor. Keep the current editor while IME occlusion proves it is
			 * open, so another tap does not cancel an active composition. */
			if (refocus && state.input && document.activeElement === state.input && !state.occluded()) {
				var previous = state.input;
				state.input = null;
				previous.blur();
				previous.remove();
			}
			if (!state.input) {
				var input = document.createElement('textarea');
				input.setAttribute('data-fb-soft-keyboard', "");
				input.setAttribute('aria-label', 'Canvas text input');
				input.autocapitalize = 'off';
				input.autocomplete = 'off';
				input.setAttribute('autocorrect', 'off');
				/* INKEY has committed bytes, without a separate preedit surface.
				 * Android's ordinary text keyboard retains English words until
				 * Space even with autocorrection disabled. Its URI layout sends
				 * raw text and retains a space bar. Other mobile layouts stay text. */
				input.inputMode = /Android/i.test(navigator.userAgent || '') ? 'url' : 'text';
				input.spellcheck = false;
				/* Keep a real editable target in the viewport. display:none and
				 * off-screen targets cannot summon the IME on mobile Safari. */
				input.style.cssText = 'position:fixed;left:0;top:0;width:1px;height:1px;opacity:.01;font-size:16px;padding:0;border:0;pointer-events:none';
				document.body.appendChild(input);
				state.input = input;
				var composing = false;
				/* Some Android IMEs do not report deletion from an empty DOM
				 * editor. Keep one invisible anchor after draining committed text.
				 * Deleting that anchor represents a BASIC Backspace, not text. */
				var anchor = '\u200B';
				var reset = function() {
					input.value = anchor;
					input.setSelectionRange(1, 1);
				};
				reset();
				var commit = function(deleting) {
					/* INKEY consumes bytes. Preserve UTF-8 without splitting a
					 * surrogate pair or submitting intermediate IME composition. */
					var text = input.value;
					if (!text.length) {
						/* Cancelling preedit can also empty the DOM editor. Only
						 * an input deletion may erase already committed BASIC text. */
						if (deleting) _fb_hJsTextInput(8);
						reset();
						return;
					}
					if (text.charAt(0) === anchor) text = text.substring(1);
					var bytes = new TextEncoder().encode(text);
					if (bytes.length <= 128)
						for (var byte of bytes) _fb_hJsTextInput(byte);
					reset();
				};
				input.addEventListener('input', function(event) {
					if (!composing && !event.isComposing)
						commit(event.inputType === 'deleteContentBackward' || (!event.inputType && !input.value.length));
				});
				/* Older mobile engines omit InputEvent.isComposing. Track the
				 * composition lifecycle as well, so prefixes are never retyped. */
				input.addEventListener('compositionstart', function() { composing = true; });
				input.addEventListener('compositionend', function() {
					composing = false;
					commit(false);
				});
				input.addEventListener('beforeinput', function(event) {
					if (composing || event.isComposing) return;
					if (event.inputType === 'insertLineBreak' || event.inputType === 'insertParagraph') {
						event.preventDefault(); _fb_hJsTextInput(13);
					}
				});
				input.addEventListener('keydown', function(event) {
					/* The editable target owns its DOM keys. Global canvas/SDL
					 * callbacks can otherwise cancel insertion before Android
					 * produces the input event which commits text to INKEY. */
					event.stopPropagation();
					if (composing || event.isComposing) return;
					/* Let Backspace delete the anchor through the DOM. Cancelling
					 * that edit leaves Android's cached IME cursor at zero, so later
					 * deleteSurroundingText requests can silently do nothing. */
					var key = {Enter:13, Tab:9, Escape:27}[event.key];
					if (key) { event.preventDefault(); _fb_hJsTextInput(key); }
				});
				input.addEventListener('keypress', function(event) { event.stopPropagation(); });
				input.addEventListener('keyup', function(event) { event.stopPropagation(); });
			}
			state.input.focus({ preventScroll: true });
		};
		/* A BASIC polling loop runs after the trusted pointer callback. Publish
		 * editable regions ahead of time so the first tap can focus synchronously.
		 * Programs without regions can request show then tap their canvas. */
		var pendingTouch = null;
		var cancelledTouch = false;
		var pointerFocus = function(event) {
			/* Use one event family per finger sequence, rather than replacing
			 * its native editor for both Pointer and Touch Events. */
			if (event.pointerType === 'touch' && window.TouchEvent) return;
			var canvas = Module['canvas'];
			if (!canvas || event.target !== canvas) return;
			if (event.touches && event.touches.length !== 1) return;
			if (event.button !== undefined && event.button !== 0) return;
			var point = event.touches ? event.touches[0] : event;
			var rect = canvas.getBoundingClientRect();
			if (rect.width <= 0 || rect.height <= 0) return;
			var x = (point.clientX - rect.left) * canvas.width / rect.width;
			var y = (point.clientY - rect.top) * canvas.height / rect.height;
			var hit = state.regions.some(function(r) {
				return x >= r[0] && y >= r[1] && x < r[0] + r[2] && y < r[1] + r[3];
			});
			var editable = state.available && (hit || (!state.regions.length && state.requested));
			var touch = event.touches || (event.pointerType && event.pointerType !== 'mouse');
			cancelledTouch = false;
			pendingTouch = touch && editable ? { x: point.screenX, y: point.screenY } : null;
			if (editable) {
				/* Android Firefox can leave a dismissed editor focused without
				 * an IME connection. Renew it during the trusted press, before
				 * canvas handlers consume the gesture. Release retries focus for
				 * engines which require activation at that end of the sequence. */
				state.focus(true, true);
				/* Prevent the canvas default action from stealing IME focus. */
				event.preventDefault();
			} else if (touch && state.input && document.activeElement === state.input) {
				/* The GUI owns non-editor focus changes. Blurring here could
				 * resize its viewport before a captured button receives release. */
				event.preventDefault();
			} else {
				state.focus(false);
				canvas.focus({ preventScroll: true });
			}
		};
		var releaseFocus = function(event) {
			if (event.pointerType === 'touch' && window.TouchEvent) return;
			if (cancelledTouch) return;
			var start = pendingTouch;
			pendingTouch = null;
			if (event.touches && event.touches.length !== 0) return;
			var point = event.changedTouches ? event.changedTouches[0] : event;
			if (!point) return;
			/* Twelve CSS pixels distinguish a tap from a scroll/selection drag.
			 * Screen coordinates survive any viewport movement during focus. */
			if (start) {
				if (Math.abs(point.screenX - start.x) > 12 || Math.abs(point.screenY - start.y) > 12) return;
			} else {
				/* Android Gecko can deliver release after a focus transition
				 * without the matching press. The GUI's pending show request
				 * and a release inside its published field still authorize IME. */
				var canvas = Module['canvas'];
				if (!state.requested || !canvas || event.target !== canvas) return;
				var bounds = canvas.getBoundingClientRect();
				if (bounds.width <= 0 || bounds.height <= 0) return;
				var x = (point.clientX - bounds.left) * canvas.width / bounds.width;
				var y = (point.clientY - bounds.top) * canvas.height / bounds.height;
				if (state.regions.length && !state.regions.some(function(r) {
					return x >= r[0] && y >= r[1] && x < r[0] + r[2] && y < r[1] + r[3];
				})) return;
			}
			state.focus(true, true);
			event.preventDefault();
		};
		document.addEventListener('pointerdown', pointerFocus, true);
		document.addEventListener('pointerup', releaseFocus, true);
		var cancelTouch = function() { pendingTouch = null; cancelledTouch = true; };
		document.addEventListener('pointercancel', cancelTouch, true);
		/* A compatibility click follows the canvas/SDL touch handlers. Restore
		 * editable focus here if those handlers reclaimed it after pointerup.
		 * Firefox Android also accepts this trusted click for IME activation. */
		document.addEventListener('click', function(event) {
			var canvas = Module['canvas'];
			if (!state.available || !canvas || event.target !== canvas) return;
			var rect = canvas.getBoundingClientRect();
			if (rect.width <= 0 || rect.height <= 0) return;
			var x = (event.clientX - rect.left) * canvas.width / rect.width;
			var y = (event.clientY - rect.top) * canvas.height / rect.height;
			if (!state.regions.some(function(r) {
				return x >= r[0] && y >= r[1] && x < r[0] + r[2] && y < r[1] + r[3];
			})) return;
			state.focus(true, true);
			event.preventDefault();
		}, false);
		{
			/* Touch Events predate Pointer Events on mobile Safari. The handler
			 * must be non-passive so the canvas cannot reclaim editable focus. */
			/* Run before the canvas/SDL touch handlers cancel the native event.
			 * Document capture was too late to renew Android Gecko's dismissed
			 * IME connection reliably. Window capture retains that activation. */
			window.addEventListener('touchstart', pointerFocus, { capture: true, passive: false });
			window.addEventListener('touchend', releaseFocus, { capture: true, passive: false });
			window.addEventListener('touchcancel', cancelTouch, true);
			if (!window.PointerEvent) document.addEventListener('mousedown', pointerFocus, true);
		}
	}
	if (what === 19) {
		var viewport = window.visualViewport;
		var width = viewport ? viewport.width : window.innerWidth;
		var height = viewport ? viewport.height : window.innerHeight;
		var host = Module['fbWindowElement'];
		if (host) {
			var bounds = host.getBoundingClientRect();
			width = Math.min(width, bounds.width);
			height = Math.min(bounds.height, height - Math.max(0, bounds.top - (viewport ? viewport.offsetTop : 0)));
		}
		var bounded = function(value) { return Number.isFinite(value) ? Math.max(0, Math.min(32767, Math.floor(value))) : 0; };
		var angle = window.screen && window.screen.orientation ? window.screen.orientation.angle : window.orientation || 0;
		HEAP32.set([bounded(width), bounded(height), bounded(96 * (window.devicePixelRatio || 1)), ((angle % 360) + 360) % 360], a >> 2);
	} else if (what === 20) {
		var visible = 0;
		if (state.occluded()) {
			visible = -1;
		} else if (state.input && document.activeElement === state.input) {
			/* Focus alone is not proof: a hardware keyboard may be connected.
			 * Older browsers expose only a viewport-occlusion heuristic. */
			visible = -2;
		}
		HEAP32.set([visible, state.available ? -1 : 0, 0, 0], a >> 2);
	} else if (what === 119) {
		state.focus(a !== 0);
	} else if (what === 120) {
		if (a >= 0 && b >= 0 && c > 0 && d > 0 && state.regions.length < 256)
			state.regions.push([a, b, c, d]);
	} else if (what === 201) {
		state.regions.length = 0;
	}
});

void fb_hMobileControl(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4)
{
	ssize_t values[4] = { 0, 0, 0, 0 };

	if (what < 100) {
		js_mobile_control(what, (int)(intptr_t)values, 0, 0, 0);
		*p1 = values[0]; *p2 = values[1];
		*p3 = values[2]; *p4 = values[3];
	} else {
		js_mobile_control(what, (int)*p1, (int)*p2, (int)*p3, (int)*p4);
	}
}
/* end of gfx_mobile.c */
