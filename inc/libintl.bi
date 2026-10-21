' This is file libintl.bi
' (FreeBasic binding for libintl version 0.18)
' Purpose: Declare gettext message catalog and translation functions.
' Responsibilities: Select the native library and preserve C argument widths.
' This file contains no catalog loader or translation storage.
'
' (C) 2011-2012 Thomas[ dot ]Freiherr[ at ]gmx[ dot ]net
' translated with help of h_2_bi.bas
' (http://www.freebasic-portal.de/downloads/ressourcencompiler/h2bi-bas-134.html)
'
' Licence:
' This library binding is free software; you can redistribute it
' and/or modify it under the terms of the GNU Lesser General Public
' License as published by the Free Software Foundation; either
' version 2 of the License, or (at your option) ANY later version.
'
' This binding is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
' Lesser General Public License for more details, refer to:
' http://www.gnu.org/licenses/lgpl.html
'
'
' Original license text:
'
'/* Message catalogs for internationalization.
   'Copyright (C) 1995-2002, 2004, 2005 Free Software Foundation, Inc.
   'This file is part of the GNU C Library.
   'This file is derived from the file libgettext.h in the GNU gettext package.

   'The GNU C Library is free software; you can redistribute it and/or
   'modify it under the terms of the GNU Lesser General Public
   'License as published by the Free Software Foundation; either
   'version 2.1 of the License, or (at your option) any later version.

   'The GNU C Library is distributed in the hope that it will be useful,
   'but WITHOUT ANY WARRANTY; without even the implied warranty of
   'MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   'Lesser General Public License for more details.

   'You should have received a copy of the GNU Lesser General Public
   'License along with the GNU C Library; if not, write to the Free
   'Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   '02111-1301 USA.  */

#IFDEF __FB_WIN32__
 #PRAGMA push(msbitfields)
 #INCLIB "iconv"
 #INCLIB "intl"
#ELSEIF DEFINED(__FB_FREEBSD__) OR DEFINED(__FB_HAIKU__)
 ' These systems provide GNU gettext in libintl rather than the C library.
 #INCLIB "intl"
#ENDIF

#INCLUDE ONCE "crt/locale.bi"
#INCLUDE ONCE "crt/long.bi"

EXTERN "C" ' (h_2_bi -P_oCD option)

' 001 start from: libintl.h2bi ==> libintl.h

#IFNDEF _LIBINTL_H
#DEFINE _LIBINTL_H 1

' file not found: features.h

#DEFINE __USE_GNU_GETTEXT 1
#DEFINE __GNU_GETTEXT_SUPPORTED_REVISION(major) IIF((major)= 0 , 1 , -1)

DECLARE FUNCTION gettext(BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR
DECLARE FUNCTION dgettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR
DECLARE FUNCTION __dgettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR
' Counts are C unsigned long, which is 64 bits on Unix x86-64 and 32 bits
' on Windows. Categories remain C int on both targets.
DECLARE FUNCTION dcgettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS LONG) AS ZSTRING PTR
DECLARE FUNCTION __dcgettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS LONG) AS ZSTRING PTR
DECLARE FUNCTION ngettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS CULONG) AS ZSTRING PTR
DECLARE FUNCTION dngettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS CULONG) AS ZSTRING PTR
DECLARE FUNCTION dcngettext(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR, BYVAL AS CULONG, BYVAL AS LONG) AS ZSTRING PTR
DECLARE FUNCTION textdomain(BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR
DECLARE FUNCTION bindtextdomain(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR
DECLARE FUNCTION bind_textdomain_codeset(BYVAL AS CONST ZSTRING PTR, BYVAL AS CONST ZSTRING PTR) AS ZSTRING PTR

#IF DEFINED (__OPTIMIZE__) AND NOT DEFINED (__cplusplus)
#DEFINE __need_NULL
#INCLUDE ONCE "crt/stddef.bi" '__HEADERS__: stddef.h

#DEFINE gettext(msgid) dgettext (NULL, msgid)
#DEFINE dgettext(domainname, msgid) dcgettext (domainname, msgid, LC_MESSAGES)
#DEFINE ngettext(msgid1, msgid2, n) dngettext (NULL, msgid1, msgid2, n)
#DEFINE dngettext(domainname, msgid1, msgid2, n) dcngettext (domainname, msgid1, msgid2, n, LC_MESSAGES)
#ENDIF ' DEFINED __OPTIM...
#ENDIF ' _LIBINTL_H

END EXTERN ' (h_2_bi -P_oCD option)

#IFDEF __FB_WIN32__
#PRAGMA pop(msbitfields)
#ENDIF

#DEFINE __(_T_) gettext(_T_)

' Translated at 11-01-28 11:16:38, by h_2_bi (version 0.2.0.1,
' released under GPLv3 by Thomas[ dot ]Freiherr[ at ]gmx[ dot ]net)

'   Protocol: libintl.bi
' Parameters:
'                                  Process time [s]: 0.006257920642383397
'                                  Bytes translated: 2184
'                                      Maximum deep: 1
'                                SUB/FUNCTION names: 11
'                                mangled TYPE names: 0
'                                        files done: 0
'                                      files missed: 2
' features.h
' locale.h
'                                                  __FOLDERS__ : 0
'                                                   __MACROS__ : 5
' 1: #define BEGIN_DECLS
' 1: #define END_DECLS
' 11: #define THROW
' 33: #define const const
' 8: #define attribute_format_arg (x)
'                                                  __HEADERS__ : 0
'                                                    __TYPES__ : 0
'                                                __POST_REPS__ : 0

' end of libintl.bi
