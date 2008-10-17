/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __CTDEF_H__
#define __CTDEF_H__

#ifndef WIN32

#ifndef DWORD_DEFINED
typedef uint32_t DWORD, *PDWORD;
#define DWORD_DEFINED 1
#endif

#ifndef LONG_DEFINED
/* BUGBUG - LONG is always 32-bits on Windows */
typedef long     LONG, *PLONG;
#define LONG_DEFINED 1
#endif

#ifndef WORD_DEFINED
typedef short WORD, *PWORD;
#define WORD_DEFINED 1
#endif

#ifndef BYTE_DEFINED
typedef unsigned char BYTE, *PBYTE;
#define BYTE_DEFINED 1
#endif

#ifndef HANDLE_DEFINED
/* BUGBUG - HANDLE should be an opaque struct poniter type */
typedef unsigned long HANDLE, *PHANDLE;
#define HANDLE_DEFINED 1
#endif

#ifndef CHAR_DEFINED
typedef char CHAR;
#define CHAR_DEFINED 1
#endif

#ifndef PSTR_DEFINED
typedef char * PSTR;
#define PSTR_DEFINED 1
#endif

#ifndef PCSTR_DEFINED
typedef const char * PCSTR;
#define PCSTR_DEFINED 1
#endif

#ifndef SOCKET_DEFINED
typedef int SOCKET;
#define SOCKET_DEFINED 1
#endif

#ifndef SOCKADDR_IN_DEFINED
typedef struct sockaddr_in SOCKADDR_IN;
#define SOCKADDR_IN_DEFINED 1
#endif

#ifndef SOCKADDR_DEFINED
typedef struct sockaddr SOCKADDR, *PSOCKADDR;
#define SOCKADDR_DEFINED 1
#endif

#ifndef VOID_DEFINED
typedef void VOID, *PVOID;
#define VOID_DEFINED 1
#endif

#ifndef HKEY_DEFINED
/* BUGBUG - HKEY should be an opaque struct pointer */
typedef PVOID HKEY, *PHKEY;
#define HKEY_DEFINED 1
#endif

#ifndef BOOLEAN_DEFINED
typedef int32_t BOOLEAN, *PBOOLEAN;
#define BOOLEAN_DEFINED 1
#endif

#ifndef ULONG_DEFINED
typedef uint32_t ULONG, *PULONG;
#define ULONG_DEFINED 1
#endif

#endif

/*
  Standard integer-related types and constants come from standard system
  headers.

  Standard integer types:
  - {u,}int{8,16,32,64}_t

  Standard MAX/MIN constants:
  - INT{8,16,32,64}_{MIN,MAX}
  - UINT{8,16,32,64}_MAX
*/

#ifndef CT_MAX
#define CT_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef CT_MIN
#define CT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct __DBLBYTE {
    BYTE b1;
    BYTE b2;
} DBLBYTE, *PDBLBYTE;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define GOTO_CLEANUP() \
    do { goto cleanup; } while (0)

#define GOTO_CLEANUP_EE(EE) \
    do { (EE) = __LINE__; goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_CENTERROR(ceError) \
    do { if (ceError) goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE) \
    do { if (ceError) { (EE) = __LINE__; goto cleanup; } } while (0)

/* Deprecated -- please use GOTO_CLEANUP versions */
#define CLEANUP_ON_CENTERROR(ceError) GOTO_CLEANUP_ON_CENTERROR(ceError)
#define CLEANUP_ON_CENTERROR_EE(ceError, EE) GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE)

#ifndef WIN32
#define BAIL_ON_CENTERIS_ERROR(__ceError__) \
    do { \
        if ((__ceError__) != 0) { \
            goto error; \
        } \
    } while (0)

#endif

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

#if defined(WORDS_BIGENDIAN)
#define CONVERT_ENDIAN_DWORD(ui32val)           \
    ((ui32val & 0x000000FF) << 24 |             \
     (ui32val & 0x0000FF00) << 8  |             \
     (ui32val & 0x00FF0000) >> 8  |             \
     (ui32val & 0xFF000000) >> 24)

#define CONVERT_ENDIAN_WORD(ui16val)            \
    ((ui16val & 0x00FF) << 8 |                  \
     (ui16val & 0xFF00) >> 8)

#else
#define CONVERT_ENDIAN_DWORD(ui32val) (ui32val)
#define CONVERT_ENDIAN_WORD(ui16val) (ui16val)
#endif

#endif /* __CTDEF_H__ */

