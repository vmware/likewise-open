/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "iop.h"

NTSTATUS
SrvDevCtlAddShare(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvDevCtlDeleteShare(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvDevCtlEnumShares(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvDevCtlSetShareInfo(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvDevCtlGetShareInfo(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvInitializeShareList(
    )
{
    NTSTATUS status = 0;

    return(ntStatus);
}


NTSTATUS
SrvFindShareinList(
    PWSTR pszShareName
    )
{
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    while (pShareEntry) {

        if (IsEqualShareEntry(pShareName pShareEntry->pName)){

            *ppShareEntry = pShareEntry;
            return(ntStatus);
        }
        pShareEntry = pShareEntry->pNext
    }
    *ppShareEntry = NULL;
    return (ntStatus);
}


NTSTATUS
SrvShareAddShare(
    LPWSTR pszShareName,
    DWORD dwLevel,
    LPVOID pBuffer
    )
{

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareinList(
                        pShareName,
                        &pShareEntry
                        );
    if (!ntStatus) {
        ntStatus = STATUS_OBJECT_EXISTS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareEntry->pNext = gpShareEntry;
    gpShareEntry = pShareEntry;

    ntStatus = SrvShareDbAdd(
                        hDb,
                        pShareEntry->pszShareName,
                        pShareEntry->pszPath,
                        pszComment
                        );

error:

    LEAVE_WRITER_LOCK();

    return(ntStatus);
}


NTSTATUS
SrvShareDeleteShare(
    LPWSTR pszShareName
    )
{

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareinList(
                        pszShareName,
                        &ppShareEntry
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SrvShareDbDelete(
                        hDb,
                        pShareEntry->pszShareName
                        );
    BAIL_ON_NT_STATUS(ntStatus);


error:

    LEAvE_WRITER_LOCK();

    return(ntStatus);

}

NTSTATUS
SrvShareSetInfo(
    LPWSTR pszShareName,
    DWORD dwLevel,
    LPVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = ValidateShareInfo(
                    dwLevel,
                    pShareInfo
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareinList(
                    pszShareName,
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwLevel) {

        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 501:
            break;

        case 502:
            break;

        case 503:
            break;

    }

    ntStatus =  SrvShareDbInsert(
                    hDb,
                    pszShareName,
                    pszPathName,
                    pszComment,
                    dwFlags,
                    pSecurityDescriptor,
                    dwSDSize
                    );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_WRITER_LOCK();

    return(ntStatus);

}


NTSTATUS
SrvShareGetInfo(
    PWSTR pszShareName,
    DWORD dwLevel,
    PBUFFER pOutBuffer,
    DWORD dwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;

    ENTER_READER_LOCK();

    ntStatus = SrvFindShareinList(
                    pszShareName,
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwLevel) {

        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 501:
            break:

        case 502:
            break;

        case 503:
            break;

    }

error:
    LEAVE_READER_LOCK();

    return(ntStatus);

}


NTSTATUS
SrvShareEnumShares(
    DWORD dwLevel,
    PBYTE pOutBuffer,
    DWORD dwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;


    ntStatus = ValidateServerSecurity(
                        hAccessToken,
                        GENERIC_READ,
                        pServerSD
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_READER_LOCK();

    ntStatus = SrvFindShareinList();

    ntStatus = SrvShareIndextoEntry(
                    dwResumeHandle,
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    while (pShareEntry) {


        switch(dwLevel) {

            case 0:
                ntStatus = GetBufferSize_L0(
                                pShareEntry,
                                &dwBufferSize
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining <  dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //
                }

                ntStatus = MarshallBuffer_L0(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;

            case 1:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;

            case 2:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
                ntStatus = GetBufferSize_L2(
                                pShareEntry,

                 break;

            case 502:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
                break;

            case 503:
               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
        }
        pShareEntry   pShareEntry->pNext;
        dwResumeHandle++;
    }


error:

    LEAVE_READER_LOCK();


    return(ntStatus);
}


NTSTATUS
SrvCreateShareEntry(
    PWSTR pszShareName,
    PWSTR pszPathName,
    PWSTR pszComment,
    PBYTE pSecurityDescriptor,
    DWORD dwSDSize,
    DWORD dwFlags
    PSRV_SHARE_ENTRY * ppShareEntry
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_OBJECT),
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszShareName,
                    &pShareEntry->pszShareName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszPathName,
                    &pShareEntry->pszPathName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszComment,
                    &pShareEntry->pszComment
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateSecurityDescriptor(
                    pSecurityDescriptor,
                    dwSDSize,
                    &pShareEntry->pSecurityDescriptor
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->dwFlags = dwFlags;

    *ppShareEntry = pShareEntry

    return(ntStatus);

error:

    if (pShareEntry) {

        SrvFreeShareEntry(pShareEntry);

    }

    *ppShareEntry = NULL;

    return(ntStatus);
}


NTSTATUS
SrvLoadSharesFromDatabase(
    )
{
    NTSTATUS ntStatus = 0;



    return(ntStatus);

}
