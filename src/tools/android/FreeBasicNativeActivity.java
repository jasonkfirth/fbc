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

public class FreeBasicNativeActivity extends NativeActivity {
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

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
