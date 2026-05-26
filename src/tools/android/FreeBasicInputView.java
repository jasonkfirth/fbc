/*
    Project: FreeBASIC Android APK Template
    ---------------------------------------

    File: FreeBasicInputView.java

    Purpose:

        Provide the hidden Java text-input view used by the Android gfx
        backend to communicate with software keyboards.

    Responsibilities:

        * behave like a normal EditText from the IME's point of view
        * catch soft-keyboard Backspace events sent to the served view
        * forward those Backspace events to the native gfx driver

    This file intentionally does NOT contain:

        * application rendering
        * generic FreeBASIC key translation
        * visible Android user interface widgets
*/

package org.freebasic.android;

import android.content.Context;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.widget.EditText;

public class FreeBasicInputView extends EditText {
    private static final int MAX_DELETE_COUNT = 16;

    public FreeBasicInputView(Context context) {
        super(context);
    }

    private static boolean dispatchBackspaceEvent(KeyEvent event) {
        if (event == null || event.getKeyCode() != KeyEvent.KEYCODE_DEL) {
            return false;
        }

        try {
            return FreeBasicNativeActivity.dispatchImeKey(event.getKeyCode(), event.getAction(), event.getUnicodeChar());
        } catch (UnsatisfiedLinkError error) {
            return false;
        }
    }

    private static boolean dispatchSyntheticBackspaces(int count) {
        int i;

        if (count <= 0) {
            return false;
        }

        /*
            InputConnection deletion requests describe how much surrounding
            text the IME wants removed.  In normal Backspace use this is one
            character, but cap it so a confused IME cannot erase an entire
            FreeBASIC input line because our hidden bridge view contains
            implementation padding.
        */
        if (count > MAX_DELETE_COUNT) {
            count = MAX_DELETE_COUNT;
        }

        try {
            for (i = 0; i < count; ++i) {
                FreeBasicNativeActivity.dispatchImeKey(KeyEvent.KEYCODE_DEL, KeyEvent.ACTION_DOWN, 0);
                FreeBasicNativeActivity.dispatchImeKey(KeyEvent.KEYCODE_DEL, KeyEvent.ACTION_UP, 0);
            }
        } catch (UnsatisfiedLinkError error) {
            return false;
        }

        return true;
    }

    private static boolean dispatchCommittedText(CharSequence text) {
        int i;
        char ch;
        boolean handled = false;

        if (text == null) {
            return false;
        }

        try {
            for (i = 0; i < text.length(); ++i) {
                ch = text.charAt(i);

                if (ch == '\n') {
                    ch = '\r';
                }

                if (ch == '\t' || ch == '\r' || (ch >= 32 && ch < 127)) {
                    FreeBasicNativeActivity.dispatchImeKey(0, KeyEvent.ACTION_DOWN, ch);
                    FreeBasicNativeActivity.dispatchImeKey(0, KeyEvent.ACTION_UP, ch);
                    handled = true;
                }
            }
        } catch (UnsatisfiedLinkError error) {
            return false;
        }

        return handled;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        /*
            Gboard sends Backspace to the served view with
            dispatchKeyEventFromInputMethod().  The NDK input queue does not
            reliably receive that event, so catch it here before EditText
            handles it internally.
        */
        if (dispatchBackspaceEvent(event)) {
            return true;
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        InputConnection base;

        base = super.onCreateInputConnection(outAttrs);
        if (base == null) {
            return null;
        }

        return new InputConnectionWrapper(base, true) {
            @Override
            public boolean sendKeyEvent(KeyEvent event) {
                if (dispatchBackspaceEvent(event)) {
                    return true;
                }

                return super.sendKeyEvent(event);
            }

            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                if (dispatchSyntheticBackspaces(beforeLength)) {
                    return true;
                }

                return super.deleteSurroundingText(beforeLength, afterLength);
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                if (dispatchCommittedText(text)) {
                    return true;
                }

                return super.commitText(text, newCursorPosition);
            }
        };
    }
}

/* end of FreeBasicInputView.java */
