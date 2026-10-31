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
        * post native-requested notifications and marshal permission prompts

    This file intentionally does NOT contain:

        * application rendering
        * FreeBASIC runtime startup
        * general keyboard translation logic
*/

package org.freebasic.android;

import android.app.NativeActivity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.View;
import android.view.ViewTreeObserver;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;

public class FreeBasicNativeActivity extends NativeActivity {
	private static final int INPUT_VIEW_ID = 0x0fb60001;
	private FreeBasicInputView keyboardInputView;
	private volatile boolean notificationsDestroyed;
	private boolean notificationPermissionPending;
	private static final int NOTIFICATION_PERMISSION_REQUEST = 0x0fb6;
	private static final String NOTIFICATION_CHANNEL = "freebasic.messages";
	private static final String NOTIFICATION_PERMISSION = "android.permission.POST_NOTIFICATIONS";
    static {
        System.loadLibrary("freebasicapp");
    }

    private static native boolean nativeDispatchImeKey(int keyCode, int action, int unicodeChar);
    private static native void nativeSetKeyboardButtonVisible(boolean visible);
    private static native void nativeWindowMetrics(int width, int height,
        int dpi, int rotation, int keyboardVisible);
    private static native void nativeInputViewReady(boolean ready);

    /* The Java UI thread observes actual layout, including Back dismissal and
     * rotation. A native show request is not evidence that an IME is visible.
     * API-30 IME queries use reflection to retain the API-23 compile contract. */
    private void publishWindowMetrics() {
        View decor = getWindow().getDecorView();
        Rect frame = new Rect();
        Rect content = new Rect();
        decor.getWindowVisibleDisplayFrame(frame);
        if (!decor.getGlobalVisibleRect(content) || !content.intersect(frame)) content.setEmpty();
        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int visible = -2;
        if (Build.VERSION.SDK_INT >= 30) {
            try {
                Object insets = View.class.getMethod("getRootWindowInsets").invoke(decor);
                Class<?> types = Class.forName("android.view.WindowInsets$Type");
                int ime = ((Integer)types.getMethod("ime").invoke(null)).intValue();
                if (insets != null) {
                    visible = ((Boolean)insets.getClass().getMethod("isVisible", int.class)
                        .invoke(insets, ime)).booleanValue() ? -1 : 0;
                }
            } catch (ReflectiveOperationException error) {
                visible = -2;
            }
        } else {
            // Older APIs cannot detect a floating IME. Report unknown when
            // focus exists without a substantial occluded area.
            int obscured = decor.getRootView().getHeight() - frame.bottom;
            if (obscured > 100 * metrics.density) visible = -1;
            else if (keyboardInputView == null || !keyboardInputView.hasFocus()) visible = 0;
        }
        try {
            nativeWindowMetrics(content.width(), content.height(), metrics.densityDpi,
                getWindowManager().getDefaultDisplay().getRotation() * 90, visible);
        } catch (UnsatisfiedLinkError error) {
            // Older runtime libraries can still use this APK template.
        }
    }

    private final ViewTreeObserver.OnGlobalLayoutListener metricsListener =
        new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override public void onGlobalLayout() { publishWindowMetrics(); }
        };

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);
        getWindow().getDecorView().post(new Runnable() {
            @Override public void run() { if (!notificationsDestroyed) publishWindowMetrics(); }
        });
    }

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

		/* A legacy native startup may have installed this reserved helper
		 * before onCreate resumed. Adopt it so one view owns IME delivery. */
		View existing = content.findViewById(INPUT_VIEW_ID);
		if (existing instanceof FreeBasicInputView) {
			keyboardInputView = (FreeBasicInputView)existing;
		} else {
			keyboardInputView = new FreeBasicInputView(this);
		}
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
		/* INKEY exposes committed text, without a separate preedit surface.
		 * Ask the IME for raw visible text. NO_SUGGESTIONS alone still lets
		 * the AOSP keyboard retain a whole unfinished word in this hidden
		 * editor, so nothing appears in the real BASIC field until Space.
		 * This is an IME hint; replacement IMEs may still use composition. */
		keyboardInputView.setInputType(
			InputType.TYPE_CLASS_TEXT |
			InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD |
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
		if (keyboardInputView.getParent() == null) {
			content.addView(keyboardInputView, new ViewGroup.LayoutParams(1, 1));
		}
		try {
			nativeInputViewReady(true);
		} catch (UnsatisfiedLinkError error) {
			/* Older runtimes retain their native text polling adapter. */
		}
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
		getWindow().getDecorView().getViewTreeObserver()
			.addOnGlobalLayoutListener(metricsListener);
		getWindow().getDecorView().post(new Runnable() {
			@Override public void run() { if (!notificationsDestroyed) publishWindowMetrics(); }
		});

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

	/* --------------------------------------------------------------------- */
	/* Notifications requested by the native runtime                          */
	/* --------------------------------------------------------------------- */

	/*
		These status numbers are the fbnotify.bi ABI. NotificationManager may
		suppress presentation after accepting a post, for example during Do Not
		Disturb. Success means accepted by the platform, not seen by the user.
		Reflection keeps the APK template buildable with an API-23 compile jar;
		channels and notification policy are queried only on supporting devices.
	*/
	private int notificationPermissionFromNative(final boolean request) {
		if (notificationsDestroyed) {
			return 1;
		}
		if (Build.VERSION.SDK_INT >= 33 &&
			checkSelfPermission(NOTIFICATION_PERMISSION) != PackageManager.PERMISSION_GRANTED) {
			if (request) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						if (!notificationsDestroyed && !notificationPermissionPending &&
							checkSelfPermission(NOTIFICATION_PERMISSION) != PackageManager.PERMISSION_GRANTED) {
							notificationPermissionPending = true;
							requestPermissions(new String[] { NOTIFICATION_PERMISSION },
								NOTIFICATION_PERMISSION_REQUEST);
						}
					}
				});
			}
			return 4;
		}
		try {
			NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
			if (manager == null) {
				return 1;
			}
			if (Build.VERSION.SDK_INT >= 24 && !((Boolean)NotificationManager.class
				.getMethod("areNotificationsEnabled").invoke(manager)).booleanValue()) {
				return 4;
			}
			/* A user can block this channel while leaving the whole app enabled.
			 * Permission queries must report that policy before a caller posts.
			 * An absent channel is allowed; the first send creates it.
			 */
			if (Build.VERSION.SDK_INT >= 26) {
				Class<?> channelClass = Class.forName("android.app.NotificationChannel");
				Object channel = NotificationManager.class.getMethod("getNotificationChannel", String.class)
					.invoke(manager, NOTIFICATION_CHANNEL);
				if (channel != null && ((Integer)channelClass.getMethod("getImportance")
					.invoke(channel)).intValue() == 0) {
					return 4;
				}
			}
			return 0;
		} catch (ReflectiveOperationException error) {
			return 3;
		}
	}

	private int notificationSendFromNative(String title, String body, int identifier) {
		int permission = notificationPermissionFromNative(false);
		if (permission != 0) {
			return permission;
		}
		try {
			NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
			Notification.Builder builder;
			if (Build.VERSION.SDK_INT >= 26) {
				Class<?> channelClass = Class.forName("android.app.NotificationChannel");
				Object channel = channelClass.getConstructor(String.class, CharSequence.class, int.class)
					.newInstance(NOTIFICATION_CHANNEL, getApplicationInfo().loadLabel(getPackageManager()), 3);
				NotificationManager.class.getMethod("createNotificationChannel", channelClass)
					.invoke(manager, channel);
				Object current = NotificationManager.class.getMethod("getNotificationChannel", String.class)
					.invoke(manager, NOTIFICATION_CHANNEL);
				if (current != null && ((Integer)channelClass.getMethod("getImportance")
					.invoke(current)).intValue() == 0) {
					return 4;
				}
				builder = Notification.Builder.class.getConstructor(android.content.Context.class, String.class)
					.newInstance(this, NOTIFICATION_CHANNEL);
			} else {
				builder = new Notification.Builder(this);
			}
			/* An immutable explicit intent reopens this APK; untrusted post text
			 * never becomes an Intent URI, package name or executable command.
			 */
			Intent intent = new Intent(this, getClass());
			intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
			int flags = PendingIntent.FLAG_UPDATE_CURRENT;
			if (Build.VERSION.SDK_INT >= 23) {
				flags |= 0x04000000; /* PendingIntent.FLAG_IMMUTABLE, added in API 23 */
			}
			builder.setSmallIcon(android.R.drawable.stat_notify_chat)
				.setContentTitle(title).setContentText(body).setAutoCancel(true)
				.setContentIntent(PendingIntent.getActivity(this, identifier, intent, flags));
			manager.notify("FreeBASIC", identifier, builder.build());
			return 0;
		} catch (SecurityException error) {
			return 4;
		} catch (ReflectiveOperationException error) {
			return 3;
		} catch (IllegalArgumentException error) {
			return 3;
		}
	}

	private int notificationCloseFromNative(int identifier) {
		NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		if (manager == null) {
			return 1;
		}
		manager.cancel("FreeBASIC", identifier);
		return 0;
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
		super.onRequestPermissionsResult(requestCode, permissions, results);
		if (requestCode == NOTIFICATION_PERMISSION_REQUEST) {
			notificationPermissionPending = false;
		}
	}

	@Override
	protected void onDestroy() {
		notificationsDestroyed = true;
		getWindow().getDecorView().getViewTreeObserver()
			.removeOnGlobalLayoutListener(metricsListener);
		try { nativeWindowMetrics(0, 0, 160, 0, 0); nativeInputViewReady(false); }
		catch (UnsatisfiedLinkError error) { /* Older native runtime. */ }
		super.onDestroy();
	}

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode;

        if (event == null) {
            return super.dispatchKeyEvent(event);
        }

        keyCode = event.getKeyCode();
        if (keyboardInputView != null && keyboardInputView.hasComposingText()) {
            return super.dispatchKeyEvent(event);
        }

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
