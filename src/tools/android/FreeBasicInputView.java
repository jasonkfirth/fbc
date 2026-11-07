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
import android.os.Build;
import android.view.inputmethod.InputMethodManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.widget.EditText;

public class FreeBasicInputView extends EditText {
    private static final int MAX_DELETE_COUNT = 16;
    private boolean clearingForwardedText;
    private boolean changingComposition;

    /* InputConnection and TextWatcher callbacks belong to Android's UI thread.
     * Preedit text stays in this helper until the IME commits it. Forwarding
     * each composing prefix would append i, im, ime for one typed word. */
    boolean hasComposingText() {
        Editable text = getText();
        return text != null && BaseInputConnection.getComposingSpanStart(text) >= 0;
    }

    private void clearForwardedText() {
        clearingForwardedText = true;
        try {
            Editable text = getText();
            if (text != null) text.clear();
        } finally {
            clearingForwardedText = false;
        }
    }

    public FreeBasicInputView(Context context) {
        super(context);
        /* This invisible transport serves every BASIC editor. It is not an
         * Android account/password field and must not summon autofill UI.
         * Reflection retains the APK template's API-23 compile contract. */
        if (Build.VERSION.SDK_INT >= 26) {
            try {
                // View.IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS, API 26.
                View.class.getMethod("setImportantForAutofill", int.class)
                    .invoke(this, 8);
            } catch (ReflectiveOperationException error) {
                // An older vendor framework can omit this optional facility.
            }
        }

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
                if (clearingForwardedText || changingComposition ||
                    text == null || text.length() == 0 || hasComposingText()) {
                    return;
                }

                dispatchCommittedText(text);
                clearForwardedText();
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

        // Match the browser commit bound. Decline oversized commits as a whole
        // rather than forwarding only the suffix that fits the native queue.
        if (text == null || text.length() > 128) {
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
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        /* Before API 30, editor focus without a docked IME is ambiguous.
         * Back release confirms dismissal: stop serving this hidden editor
         * so layout observation can report hidden instead of unknown. Handle
         * the release here so dismissing the IME does not close the activity.
         * Modern releases retain their authoritative WindowInsets path. */
        if (Build.VERSION.SDK_INT < 30 && keyCode == KeyEvent.KEYCODE_BACK &&
            event != null && event.getAction() == KeyEvent.ACTION_UP &&
            !event.isCanceled() && hasFocus()) {
            InputMethodManager manager = (InputMethodManager)getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
            if (manager != null) {
                manager.hideSoftInputFromWindow(getWindowToken(), 0);
            }
            clearFocus();
            setFocusable(false);
            setFocusableInTouchMode(false);
            return true;
        }
        return super.onKeyPreIme(keyCode, event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        /*
            Gboard sends Backspace to the served view with
            dispatchKeyEventFromInputMethod().  The NDK input queue does not
            reliably receive that event, so catch it here before EditText
            handles it internally.
        */
        if (!hasComposingText() && dispatchNativeKeyEvent(event)) {
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
                if (!hasComposingText() && dispatchNativeKeyEvent(event)) {
                    return true;
                }

                return super.sendKeyEvent(event);
            }

            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                if (hasComposingText()) {
                    return super.deleteSurroundingText(beforeLength, afterLength);
                }
                if (dispatchSyntheticBackspaces(beforeLength)) {
                    return true;
                }

                return super.deleteSurroundingText(beforeLength, afterLength);
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                if (dispatchCommittedText(text)) {
                    clearForwardedText();
                    return true;
                }

                return super.commitText(text, newCursorPosition);
            }

            @Override
            public boolean setComposingText(CharSequence text, int newCursorPosition) {
                changingComposition = true;
                try {
                    return super.setComposingText(text, newCursorPosition);
                } finally {
                    changingComposition = false;
                }
            }

            @Override
            public boolean finishComposingText() {
                String pending = hasComposingText() ? getText().toString() : "";
                boolean result = super.finishComposingText();
                if (pending.length() != 0 && dispatchCommittedText(pending)) {
                    clearForwardedText();
                }
                return result;
            }
        };
    }
}

/* end of FreeBasicInputView.java */
