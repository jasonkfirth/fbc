/*
 * Project: FreeBASIC C source lint
 * File: cppcheck-platform-model.h
 * Purpose: Describe compiler extensions that are not C source bodies.
 * Responsibilities: Supply GNU label-prefix syntax and Emscripten signatures.
 * This file intentionally does NOT suppress C data-flow diagnostics.
 */
#ifndef FB_CPPCHECK_PLATFORM_MODEL_H
#define FB_CPPCHECK_PLATFORM_MODEL_H
#ifndef FB_CPPCHECK_ANALYSIS
#error This header is only for Cppcheck analysis
#endif

/* The lint runner uses a generic GNU C model, without an object-file linker. */
#ifndef __USER_LABEL_PREFIX__
#define __USER_LABEL_PREFIX__
#endif

/* EM_JS bodies are JavaScript, not C. Preserve their C-facing signatures;
 * browser tests exercise the JavaScript implementation separately. */
#ifndef EM_JS
#define EM_JS(result, name, parameters, ...) result name parameters;
#endif

/* These standalone fixtures normally receive record tags and character width
 * from their compiler test drivers. Give the lint-only configuration names. */
#ifndef FB_TEST_RECORD
#define FB_TEST_RECORD fb_cppcheck_anonymous_record
#endif
#ifndef FB_TEST_OPAQUE
#define FB_TEST_OPAQUE fb_cppcheck_opaque_record
#endif
#ifndef FB_TEST_POINTER_HEADER
#define FB_TEST_POINTER_HEADER fb_cppcheck_pointer_header
#endif
#ifndef FB_TEST_COMPLETED
#define FB_TEST_COMPLETED fb_cppcheck_completed_record
#endif
#ifndef FB_TEST_WCHAR_WIDTH
#define FB_TEST_WCHAR_WIDTH 4
#endif

#endif
/* end of cppcheck-platform-model.h */
