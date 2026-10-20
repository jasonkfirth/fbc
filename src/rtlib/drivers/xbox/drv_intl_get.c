/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/drv_intl_get.c
 * Purpose: get i18n data.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

const char *fb_DrvIntlGet( eFbIntlIndex Index )
{
	/* No I18N information available */
	return NULL;
}

/* end of drivers/xbox/drv_intl_get.c */
