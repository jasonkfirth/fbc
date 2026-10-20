/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/drv_intl_data.c
 * Purpose: helper functions for internationalization support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"

const FB_LOCALE_INFOS __fb_locale_infos[] = {
    {
        49,         /* German */
        {
            "Januar",
            "Februar",
            "M„rz",
            "April",
            "Mai",
            "Juni",
            "Juli",
            "August",
            "September",
            "Oktober",
            "November",
            "Dezember"
        },
        {
            "Jan",
            "Feb",
            "M„r",
            "Apr",
            "Mai",
            "Jun",
            "Jul",
            "Aug",
            "Sep",
            "Okt",
            "Nov",
            "Dez"
        },
        {
            "Sonntag",
            "Montag",
            "Dienstag",
            "Mittwoch",
            "Donnerstag",
            "Freitag",
            "Samstag"
        },
        {
            "So",
            "Mo",
            "Di",
            "Mi",
            "Do",
            "Fr",
            "Sa"
        }
    },
    {
        -1,
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        },
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        },
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        },
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        }
    }
};

const size_t         __fb_locale_info_count = sizeof(__fb_locale_infos) / sizeof(FB_LOCALE_INFOS);

/* end of drivers/dos/drv_intl_data.c */
