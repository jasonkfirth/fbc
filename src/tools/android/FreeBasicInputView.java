/*
    Project: FreeBASIC Android APK Template
    ---------------------------------------

    File: FreeBasicInputView.java

    Purpose:

        Provide the hidden Java text-input view used by the Android gfx
        backend to communicate with software keyboards.

    Responsibilities:

        * behave like a normal EditText from the IME's point of view
        * catch key events sent to the served view by software or hardware
          keyboards
        * forward supported FreeBASIC text input to the native gfx driver

    This file intentionally does NOT contain:

        * application rendering
        * generic FreeBASIC key translation
        * visible Android user interface widgets
*/

package org.freebasic.android;

import android.content.Context;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.widget.EditText;

public class FreeBasicInputView extends EditText {
    private static final int MAX_DELETE_COUNT = 16;
    private boolean clearingForwardedText;

    public FreeBasicInputView(Context context) {
        super(context);

        /*
            Some older Android IMEs bypass the InputConnection wrapper and
            modify their served EditText directly.  Keep the helper view
            empty, but treat that otherwise invisible change as a fallback
            text commit.  The normal wrapper path returns true before this
            watcher sees a change, so it cannot duplicate ordinary input.
        */
        addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence text, int start,
                int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence text, int start,
                int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable text) {
                if (clearingForwardedText || text == null || text.length() == 0) {
                    return;
                }

                dispatchCommittedText(text);
                clearingForwardedText = true;
                text.clear();
                clearingForwardedText = false;
            }
        });
    }

    private static boolean dispatchNativeKeyEvent(KeyEvent event) {
        int keyCode;
        int unicodeChar;

        if (event == null) {
            return false;
        }

        keyCode = event.getKeyCode();
        unicodeChar = event.getUnicodeChar();
        if (keyCode != KeyEvent.KEYCODE_DEL &&
            unicodeChar != '\t' && unicodeChar != '\r' && unicodeChar != '\n' &&
            (unicodeChar < 32 || unicodeChar >= 127)) {
            return false;
        }

        try {
            return FreeBasicNativeActivity.dispatchImeKey(
                keyCode,
                event.getAction(),
                unicodeChar
            );
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
        if (dispatchNativeKeyEvent(event)) {
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
                if (dispatchNativeKeyEvent(event)) {
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
