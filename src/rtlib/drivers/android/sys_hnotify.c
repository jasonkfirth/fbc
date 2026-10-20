/*
 * Project: FreeBASIC Android runtime: sys_hnotify.c
 * Purpose: Bridge the portable notification ABI to the APK activity.
 * Responsibilities: JNI thread attachment, activity references and UTF-16.
 * Android UI calls, channels and permission prompts belong to the Java activity.
 * File: drivers/android/sys_hnotify.c
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#include "../../fb_services.h"
#include <android/native_activity.h>
#include <pthread.h>

typedef struct FB_NOTIFY_ANDROID {
	pthread_t owner;
	JavaVM *vm;
	jobject activity;
	jmethodID send;
	jmethodID close;
	jmethodID permission;
	jint identifier;
} FB_NOTIFY_ANDROID;

/* The template calls the bind hook on Android's UI thread. A global reference
 * is necessary: the BASIC thread outlives a JNI local-reference frame. Contexts
 * take their own references, and Java refuses work after activity destruction.
 */
static pthread_mutex_t activity_mutex = PTHREAD_MUTEX_INITIALIZER;
static JavaVM *activity_vm;
static jobject activity_object;
static jint next_identifier;

void fb_hNotifyAndroidSetActivity(ANativeActivity *activity)
{
	JNIEnv *environment = NULL;
	pthread_mutex_lock(&activity_mutex);
	if (activity_vm != NULL && activity_object != NULL &&
	    (*activity_vm)->GetEnv(activity_vm, (void **)&environment,
		JNI_VERSION_1_6) == JNI_OK)
		(*environment)->DeleteGlobalRef(environment, activity_object);
	activity_vm = NULL;
	activity_object = NULL;
	if (activity != NULL) {
		environment = activity->env;
		activity_object = (*environment)->NewGlobalRef(environment, activity->clazz);
		if ((*environment)->ExceptionCheck(environment)) {
			(*environment)->ExceptionClear(environment);
			activity_object = NULL;
		}
		if (activity_object != NULL)
			activity_vm = activity->vm;
	}
	pthread_mutex_unlock(&activity_mutex);
}

static JNIEnv *attach(JavaVM *vm, int *attached)
{
	JNIEnv *environment = NULL;
	jint result;
	*attached = FALSE;
	result = (*vm)->GetEnv(vm, (void **)&environment, JNI_VERSION_1_6);
	if (result == JNI_EDETACHED) {
		pthread_attr_t attributes;
		size_t stack_size;
		int stack_result;
		/* ART reserves stack space while attaching a native thread. Some
		 * Android releases abort the process on BASIC's small default worker
		 * stack instead of returning a JNI error. Refuse before entering ART;
		 * notification workers must reserve at least one MiB with ThreadCreate.
		 * Already attached Java/UI threads have the VM's own stack policy.
		 */
		if (pthread_getattr_np(pthread_self(), &attributes) != 0)
			return NULL;
		stack_result = pthread_attr_getstacksize(&attributes, &stack_size);
		pthread_attr_destroy(&attributes);
		if (stack_result != 0 || stack_size < 1024u * 1024u)
			return NULL;
		if ((*vm)->AttachCurrentThread(vm, &environment, NULL) != JNI_OK)
			return NULL;
		*attached = TRUE;
	} else if (result != JNI_OK) {
		return NULL;
	}
	return environment;
}

static void detach(JavaVM *vm, int attached)
{
	if (attached)
		(*vm)->DetachCurrentThread(vm);
}

static jstring utf8_string(JNIEnv *environment, const char *text)
{
	jchar units[FB_NOTIFY_BODY_MAX + 1u];
	jsize count = 0;
	const unsigned char *next = (const unsigned char *)text;
	/* NewStringUTF expects JNI's modified UTF-8. Convert standard UTF-8
	 * ourselves so supplementary characters survive the BASIC/JNI boundary.
	 * The public entry already checked encodings and the byte count.
	 */
	while (*next != 0) {
		uint32_t value = *next++;
		unsigned int remaining = 0;
		if (value >= 0xf0u) { value &= 7u; remaining = 3; }
		else if (value >= 0xe0u) { value &= 15u; remaining = 2; }
		else if (value >= 0xc0u) { value &= 31u; remaining = 1; }
		while (remaining-- != 0)
			value = (value << 6) | (*next++ & 63u);
		if (value > 0xffffu) {
			value -= 0x10000u;
			units[count++] = (jchar)(0xd800u + (value >> 10));
			units[count++] = (jchar)(0xdc00u + (value & 1023u));
		} else {
			units[count++] = (jchar)value;
		}
	}
	return (*environment)->NewString(environment, units, count);
}

int fb_hNotifyCreate(const char *app_name, void **output)
{
	FB_NOTIFY_ANDROID *context = calloc(1, sizeof(*context));
	JNIEnv *environment = NULL;
	jclass activity_class;
	int attached = FALSE;
	(void)app_name; /* Android identifies notifications by the APK label. */
	if (context == NULL)
		return FB_SERVICE_FAILED;
	pthread_mutex_lock(&activity_mutex);
	context->vm = activity_vm;
	if (context->vm != NULL && next_identifier < INT_MAX) {
		environment = attach(context->vm, &attached);
		if (environment != NULL) {
			context->activity = (*environment)->NewGlobalRef(environment, activity_object);
			context->identifier = ++next_identifier;
		}
	}
	pthread_mutex_unlock(&activity_mutex);
	if (environment == NULL)
		goto fail;
	if (context->activity == NULL)
		goto fail;
	activity_class = (*environment)->GetObjectClass(environment, context->activity);
	if (activity_class == NULL)
		goto fail;
	context->send = (*environment)->GetMethodID(environment, activity_class,
		"notificationSendFromNative", "(Ljava/lang/String;Ljava/lang/String;I)I");
	if (!(*environment)->ExceptionCheck(environment))
		context->close = (*environment)->GetMethodID(environment, activity_class,
			"notificationCloseFromNative", "(I)I");
	if (!(*environment)->ExceptionCheck(environment))
		context->permission = (*environment)->GetMethodID(environment, activity_class,
			"notificationPermissionFromNative", "(Z)I");
	(*environment)->DeleteLocalRef(environment, activity_class);
	if (context->send == NULL || context->close == NULL || context->permission == NULL)
		goto fail;
	context->owner = pthread_self();
	detach(context->vm, attached);
	*output = context;
	return FB_SERVICE_OK;
fail:
	if (environment != NULL) {
		if ((*environment)->ExceptionCheck(environment))
			(*environment)->ExceptionClear(environment);
		if (context->activity != NULL)
			(*environment)->DeleteGlobalRef(environment, context->activity);
		detach(context->vm, attached);
	}
	free(context);
	return FB_SERVICE_UNAVAILABLE;
}

int fb_hNotifySend(void *opaque, const char *title, const char *body)
{
	FB_NOTIFY_ANDROID *context = opaque;
	JNIEnv *environment;
	jstring java_title = NULL, java_body = NULL;
	int attached, result = FB_SERVICE_FAILED;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	environment = attach(context->vm, &attached);
	if (environment == NULL)
		return FB_SERVICE_FAILED;
	java_title = utf8_string(environment, title);
	if (java_title != NULL)
		java_body = utf8_string(environment, body);
	if (java_title != NULL && java_body != NULL)
		result = (*environment)->CallIntMethod(environment, context->activity,
			context->send, java_title, java_body, context->identifier);
	if ((*environment)->ExceptionCheck(environment)) {
		(*environment)->ExceptionClear(environment);
		result = FB_SERVICE_FAILED;
	}
	if (java_title != NULL) (*environment)->DeleteLocalRef(environment, java_title);
	if (java_body != NULL) (*environment)->DeleteLocalRef(environment, java_body);
	detach(context->vm, attached);
	return result;
}

static int simple_call(FB_NOTIFY_ANDROID *context, int permission, int request)
{
	JNIEnv *environment;
	int attached, result;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	environment = attach(context->vm, &attached);
	if (environment == NULL)
		return FB_SERVICE_FAILED;
	if (permission)
		result = (*environment)->CallIntMethod(environment, context->activity,
			context->permission, request ? JNI_TRUE : JNI_FALSE);
	else
		result = (*environment)->CallIntMethod(environment, context->activity,
			context->close, context->identifier);
	if ((*environment)->ExceptionCheck(environment)) {
		(*environment)->ExceptionClear(environment);
		result = FB_SERVICE_FAILED;
	}
	detach(context->vm, attached);
	return result;
}

int fb_hNotifyPoll(void *opaque)
{
	return simple_call(opaque, TRUE, FALSE);
}
int fb_hNotifyPermission(void *opaque, int request)
{
	return simple_call(opaque, TRUE, request);
}
int fb_hNotifyClose(void *opaque)
{
	return simple_call(opaque, FALSE, FALSE);
}
int fb_hNotifyDestroy(void *opaque)
{
	FB_NOTIFY_ANDROID *context = opaque;
	JNIEnv *environment;
	int attached;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	(void)fb_hNotifyClose(context);
	environment = attach(context->vm, &attached);
	if (environment == NULL)
		return FB_SERVICE_FAILED;
	(*environment)->DeleteGlobalRef(environment, context->activity);
	detach(context->vm, attached);
	free(context);
	return FB_SERVICE_OK;
}

/* end of drivers/android/sys_hnotify.c */
