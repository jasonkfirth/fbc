/*
    Project: FreeBASIC Android APK Template
    ---------------------------------------

    File: FreeBasicInputBridge.java

    Purpose:

        Attach small Java-side input hooks to the hidden EditText used by the
        Android gfx backend for software keyboard input.

    Responsibilities:

        * keep the platform-facing view as a normal Android EditText
        * catch IME-dispatched Backspace key events
        * forward those events to the native gfx driver

    This file intentionally does NOT contain:

        * application rendering
        * visible Android user interface widgets
        * general keyboard layout translation
*/

package org.freebasic.android;

import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;

public final class FreeBasicInputBridge {
    private FreeBasicInputBridge() {
    }

    public static void attach(EditText view) {
        if (view == null) {
            return;
        }

        view.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (event == null || keyCode != KeyEvent.KEYCODE_DEL) {
                    return false;
                }

                try {
                    return FreeBasicNativeActivity.dispatchImeKey(
                        keyCode,
                        event.getAction(),
                        event.getUnicodeChar()
                    );
                } catch (UnsatisfiedLinkError error) {
                    return false;
                }
            }
        });
    }
}

/* end of FreeBasicInputBridge.java */
