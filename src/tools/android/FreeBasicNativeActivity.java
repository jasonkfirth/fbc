/*
    Project: FreeBASIC Android APK Template
    ---------------------------------------

    File: FreeBasicNativeActivity.java

    Purpose:

        Provide the small Java side of the Android NativeActivity wrapper.

    Responsibilities:

        * inherit Android's NativeActivity loader and lifecycle plumbing
        * catch soft-keyboard Backspace events that Android dispatches
          through the Java view hierarchy
        * forward those events to the native FreeBASIC Android gfx driver

    This file intentionally does NOT contain:

        * application rendering
        * FreeBASIC runtime startup
        * general keyboard translation logic
*/

package org.freebasic.android;

import android.app.NativeActivity;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;

public class FreeBasicNativeActivity extends NativeActivity {
	private static final int INPUT_VIEW_ID = 0x0fb60001;
	private FreeBasicInputView keyboardInputView;
    static {
        System.loadLibrary("freebasicapp");
    }

    private static native boolean nativeDispatchImeKey(int keyCode, int action, int unicodeChar);
    private static native void nativeSetKeyboardButtonVisible(boolean visible);

    static boolean dispatchImeKey(int keyCode, int action, int unicodeChar) {
        return nativeDispatchImeKey(keyCode, action, unicodeChar);
    }

    private static boolean isPrintableUnicode(int unicodeChar) {
        return unicodeChar == '\t' || unicodeChar == '\r' || unicodeChar == '\n' ||
            (unicodeChar >= 32 && unicodeChar < 127);
    }

    private boolean readKeyboardButtonVisible() {
        ActivityInfo info;
        Bundle metaData;

        try {
            info = getPackageManager().getActivityInfo(getComponentName(), PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException error) {
            return true;
        }

        metaData = info.metaData;
        if (metaData == null) {
            return true;
        }

        /*
            This is a build-time policy switch for programs that never need
            text entry.  It keeps the native renderer simple: the button is
            either available for the whole app or omitted for the whole app.
        */
        return metaData.getBoolean("org.freebasic.android.keyboard_button", true);
    }

	/*
		NativeActivity's graphics surface is not a reliable IME target on every
		Android release.  Keep a one-pixel, transparent EditText in the activity
		content hierarchy instead.  The view is deliberately unfocused until the
		GPU-rendered KB control is tapped, so starting a graphics program never
		causes a keyboard or handwriting panel to appear unexpectedly.
	*/
	private void installKeyboardInputView() {
		ViewGroup content;

		content = (ViewGroup)findViewById(android.R.id.content);
		if (content == null || keyboardInputView != null) {
			return;
		}

		keyboardInputView = new FreeBasicInputView(this);
		keyboardInputView.setId(INPUT_VIEW_ID);
		/*
			Do not let this one-pixel helper win Android's initial focus search.
			It becomes focusable only for an explicit renderer-originated show
			request, then returns to this inactive state after hiding the IME.
		*/
		keyboardInputView.setFocusable(false);
		keyboardInputView.setFocusableInTouchMode(false);
		keyboardInputView.setBackgroundColor(0);
		keyboardInputView.setAlpha(0.01f);
		keyboardInputView.setInputType(
			InputType.TYPE_CLASS_TEXT |
			InputType.TYPE_TEXT_FLAG_MULTI_LINE |
			InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
		);
		keyboardInputView.setImeOptions(
			EditorInfo.IME_ACTION_NONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI
		);
		keyboardInputView.setSingleLine(false);
		keyboardInputView.setCursorVisible(false);
		keyboardInputView.setTextColor(0);
		FreeBasicInputBridge.attach(keyboardInputView);
		content.addView(keyboardInputView, new ViewGroup.LayoutParams(1, 1));
	}

	/*
		Called from the native input thread after a touch has been consumed by the
		gfxlib3 presentation overlay.  runOnUiThread is necessary because the
		InputMethodManager and the served view belong to Android's UI thread, not
		the renderer's EGL thread.
	*/
	private void setKeyboardVisibleFromNative(final boolean visible) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				InputMethodManager inputManager;

				installKeyboardInputView();
				if (keyboardInputView == null) {
					return;
				}
				inputManager = (InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
				if (inputManager == null) {
					return;
				}
				if (visible) {
					keyboardInputView.setFocusable(true);
					keyboardInputView.setFocusableInTouchMode(true);
					keyboardInputView.requestFocus();
					inputManager.showSoftInput(keyboardInputView, 0);
				} else {
					inputManager.hideSoftInputFromWindow(
						keyboardInputView.getWindowToken(), 0);
					keyboardInputView.clearFocus();
					keyboardInputView.setFocusable(false);
					keyboardInputView.setFocusableInTouchMode(false);
				}
			}
		});
	}

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		installKeyboardInputView();

        try {
            nativeSetKeyboardButtonVisible(readKeyboardButtonVisible());
        } catch (UnsatisfiedLinkError error) {
            /*
                NativeActivity normally loads libfreebasicapp.so before this
                point.  If a device changes that ordering, leave the default
                native setting alone rather than failing activity startup.
            */
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode;

        if (event == null) {
            return super.dispatchKeyEvent(event);
        }

        keyCode = event.getKeyCode();

        /*
            Some keyboards deliver printable keys through the Java dispatch
            path instead of committing text to the hidden EditText.  Forward
            those keys with Android's translated Unicode character so the
            native driver does not have to guess from raw keycodes.
        */
        if (keyCode == KeyEvent.KEYCODE_DEL || isPrintableUnicode(event.getUnicodeChar())) {
            try {
                if (nativeDispatchImeKey(keyCode, event.getAction(), event.getUnicodeChar())) {
                    return true;
                }
            } catch (UnsatisfiedLinkError error) {
                /*
                    The NativeActivity base class loads libfreebasicapp.so.
                    If Android delivers an unusually early key event before
                    that library is ready, fall through to the platform
                    handler rather than dropping the event.
                */
            }
        }

        return super.dispatchKeyEvent(event);
    }
}

/* end of FreeBasicNativeActivity.java */
