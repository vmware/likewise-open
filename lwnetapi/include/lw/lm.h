/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lm.h
 *
 * Abstract:
 *
 *        Likewise Net API
 *
 *        Public API
 *
 */
#ifndef __LM_H__
#define __LM_H__

#ifndef NET_API_STATUS_DEFINED
typedef WINERROR NET_API_STATUS;

#define NET_API_STATUS_DEFINED
#endif

NET_API_STATUS
NetApiInitialize(
    VOID
    );

NET_API_STATUS
NetApiBufferAllocate(
    DWORD  dwCount,
    PVOID* ppBuffer
    );

NET_API_STATUS
NetApiBufferFree(
    PVOID pBuffer
    );

NET_API_STATUS
NetServerGetInfoA(
    PSTR   pszServername,  /* IN    OPTIONAL */
    DWORD  dwInfoLevel,    /* IN             */
    PBYTE* ppBuffer        /*    OUT         */
    );

NET_API_STATUS
NetServerGetInfoW(
    PWSTR  pwszServername, /* IN    OPTIONAL */
    DWORD  dwInfoLevel,    /* IN             */
    PBYTE* ppBuffer        /*    OUT         */
    );

NET_API_STATUS
NetServerSetInfoA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT OPTIONAL */
    );

NET_API_STATUS
NetServerSetInfoW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT OPTIONAL */
    );

NET_API_STATUS
NetShareEnumA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer,       /*    OUT          */
    DWORD  dwPrefmaxLen,   /* IN              */
    PDWORD pdwEntriesRead, /*    OUT          */
    PDWORD pdwTotalEntries,/*    OUT          */
    PDWORD pdwResumeHandle /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetShareEnumW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer,       /*    OUT          */
    DWORD  dwPrefmaxLen,   /* IN              */
    PDWORD pdwEntriesRead, /*    OUT          */
    PDWORD pdwTotalEntries,/*    OUT          */
    PDWORD pdwResumeHandle /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetShareGetInfoA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    PSTR   pszNetname,     /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer        /*    OUT          */
    );

NET_API_STATUS
NetShareGetInfoW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    PWSTR  pwszNetname,    /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer        /*    OUT          */
    );

NET_API_STATUS
NetShareSetInfoA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    PSTR   pszNetname,     /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT          */
    );

NET_API_STATUS
NetShareSetInfoW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    PWSTR  pwszNetname,    /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT          */
    );

NET_API_STATUS
NetShareAddA(
    PSTR    pszServername,  /* IN     OPTIONAL */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    );

NET_API_STATUS
NetShareAddW(
    PWSTR   pwszServername, /* IN     OPTIONAL */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    );

NET_API_STATUS
NetShareDelA(
    PSTR    pszServername,  /* IN     OPTIONAL */
    PSTR    pszNetname,     /* IN              */
    DWORD   dwReserved      /* IN              */
    );

NET_API_STATUS
NetShareDelW(
    PWSTR   pwszServername, /* IN     OPTIONAL */
    PWSTR   pwszNetname,    /* IN              */
    DWORD   dwReserved      /* IN              */
    );

NET_API_STATUS
NetSessionEnumA(
    PSTR    pszServername,    /* IN     OPTIONAL */
    PSTR    pszUncClientname, /* IN     OPTIONAL */
    PSTR    pszUsername,      /* IN     OPTIONAL */
    DWORD   dwInfoLevel,      /* IN              */
    PBYTE*  ppBuffer,         /*    OUT          */
    DWORD   dwPrefmaxLen,     /* IN              */
    PDWORD  pdwEntriesRead,   /*    OUT          */
    PDWORD  pdwTotalEntries,  /*    OUT          */
    PDWORD  pdwResumeHandle   /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetSessionEnumW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszUncClientname, /* IN     OPTIONAL */
    PWSTR   pwszUsername,      /* IN     OPTIONAL */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxLen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetSessionDelA(
    PSTR    pszServername,     /* IN     OPTIONAL */
    PSTR    pszUncClientname,  /* IN     OPTIONAL */
    PSTR    pszUsername        /* IN     OPTIONAL */
    );

NET_API_STATUS
NetSessionDelW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszUncClientname, /* IN     OPTIONAL */
    PWSTR   pwszUsername       /* IN     OPTIONAL */
    );

NET_API_STATUS
NetConnectionEnumA(
    PSTR    pszServername,     /* IN     OPTIONAL */
    PSTR    pszQualifier,      /* IN              */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxlen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetConnectionEnumW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszQualifier,     /* IN              */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxlen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetFileEnumA(
    PSTR   pszServername,      /* IN     OPTIONAL */
    PSTR   pszBasepath,        /* IN     OPTIONAL */
    PSTR   pszUsername,        /* IN     OPTIONAL */
    DWORD  dwInfoLevel,        /* IN              */
    PBYTE* ppBuffer,           /*    OUT          */
    DWORD  dwPrefmaxlen,       /* IN              */
    PDWORD pwdEntriesRead,     /*    OUT          */
    PDWORD pdwTotalEntries,    /*    OUT          */
    PDWORD pdwResumeHandle     /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetFileEnumW(
    PWSTR  pwszServername,     /* IN    OPTIONAL  */
    PWSTR  pwszBasepath,       /* IN    OPTIONAL  */
    PWSTR  pwszUsername,       /* IN    OPTIONAL  */
    DWORD  dwInfoLevel,        /* IN              */
    PBYTE* ppBuffer,           /*    OUT          */
    DWORD  dwPrefmaxlen,       /* IN              */
    PDWORD pwdEntriesRead,     /*    OUT          */
    PDWORD pdwTotalEntries,    /*    OUT          */
    PDWORD pdwResumeHandle     /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetFileCloseA(
    PSTR   pszServername,      /* IN    OPTIONAL  */
    DWORD  dwFileId            /* IN              */
    );

NET_API_STATUS
NetFileCloseW(
    PWSTR  pwszServername,     /* IN    OPTIONAL  */
    DWORD  dwFileId            /* IN              */
    );

NET_API_STATUS
NetRemoteTODA(
    PSTR   pszServername,      /* IN    OPTIONAL  */
    PBYTE* ppBuffer            /*    OUT          */
    );

NET_API_STATUS
NetRemoteTODW(
    PWSTR  pwszServername,     /* IN    OPTIONAL  */
    PBYTE* ppBuffer            /*    OUT          */
    );

NET_API_STATUS
NetApiShutdown(
    VOID
    );

#if defined(UNICODE)

#define NetServerGetInfo(Arg1, Arg2, Arg3) \
        NetServerGetInfoW(Arg1, Arg2, Arg3)

#define NetServerSetInfo(Arg1, Arg2, Arg3, Arg4) \
        NetServerSetInfoW(Arg1, Arg2, Arg3, Arg4)

#define NetShareEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7) \
        NetShareEnumW(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)

#define NetShareGetInfo(Arg1, Arg2, Arg3, Arg4) \
        NetShareGetInfoW(Arg1, Arg2, Arg3, Arg4)

#define NetShareSetInfo(Arg1, Arg2, Arg3, Arg4, Arg5) \
        NetShareSetInfoW(Arg1, Arg2, Arg3, Arg4, Arg5)

#define NetShareAdd(Arg1, Arg2, Arg3, Arg4) \
        NetShareAddW(Arg1, Arg2, Arg3, Arg4)

#define NetServerDel(Arg1, Arg2, Arg3) \
        NetServerDelW(Arg1, Arg2, Arg3)

#define NetSessionEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9) \
        NetSessionEnumW(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)

#define NetSessionDel(Arg1, Arg2, Arg3) \
        NetSessionDelW(Arg1, Arg2, Arg3)

#define NetConnectionEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8) \
        NetConnectionEnumW(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8)

#define NetFileEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9) \
        NetFileEnumW(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)

#define NetFileClose(Arg1, Arg2) \
        NetFileCloseW(Arg1, Arg2)

#define NetRemoteTOD(Arg1, Arg2) \
        NetRemoteTODW(Arg1, Arg2)

#else

#define NetServerGetInfo(Arg1, Arg2, Arg3) \
        NetServerGetInfoA(Arg1, Arg2, Arg3)

#define NetServerSetInfo(Arg1, Arg2, Arg3, Arg4) \
        NetServerSetInfoA(Arg1, Arg2, Arg3, Arg4)

#define NetShareEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7) \
        NetShareEnumA(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)

#define NetShareGetInfo(Arg1, Arg2, Arg3, Arg4) \
        NetShareGetInfoA(Arg1, Arg2, Arg3, Arg4)

#define NetShareSetInfo(Arg1, Arg2, Arg3, Arg4, Arg5) \
        NetShareSetInfoA(Arg1, Arg2, Arg3, Arg4, Arg5)

#define NetShareAdd(Arg1, Arg2, Arg3, Arg4) \
        NetShareAddA(Arg1, Arg2, Arg3, Arg4)

#define NetServerDel(Arg1, Arg2, Arg3) \
        NetServerDelA(Arg1, Arg2, Arg3)

#define NetSessionEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9) \
        NetSessionEnumA(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)

#define NetSessionDel(Arg1, Arg2, Arg3) \
        NetSessionDelA(Arg1, Arg2, Arg3)

#define NetConnectionEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8) \
        NetConnectionEnumA(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8)

#define NetFileEnum(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9) \
        NetFileEnumA(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)

#define NetFileClose(Arg1, Arg2) \
        NetFileCloseA(Arg1, Arg2)

#define NetRemoteTOD(Arg1, Arg2) \
        NetRemoteTODA(Arg1, Arg2)

#endif /* UNICODE */

#endif /* __LM_H__ */
