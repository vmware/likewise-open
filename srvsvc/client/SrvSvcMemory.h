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

#ifndef _SRVSVC_MEMORY_H_
#define _SRVSVC_MEMORY_H_

NET_API_STATUS SrvSvcInitMemory(void);

NET_API_STATUS SrvSvcDestroyMemory(void);

NET_API_STATUS SrvSvcAllocateMemory(void **ptr, size_t len, void *dep);

NET_API_STATUS SrvSvcFreeMemory(void *ptr);

NET_API_STATUS SrvSvcAddDepMemory(void *ptr, void *dep);

NET_API_STATUS SrvSvcCopyNetConnCtr(uint32 level, srvsvc_NetConnCtr *ctr,
                                    uint32 *entriesread, uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetFileCtr(uint32 level, srvsvc_NetFileCtr *ctr,
                                    uint32 *entriesread, uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetFileInfo(uint32 level, srvsvc_NetFileInfo *info,
                                     uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetSessCtr(uint32 level, srvsvc_NetSessCtr *ctr,
                                    uint32 *entriesread, uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetShareCtr(uint32 level, srvsvc_NetShareCtr *ctr,
                                     uint32 *entriesread, uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetShareInfo(uint32 level, srvsvc_NetShareInfo *info,
                                      uint8 **bufptr);

NET_API_STATUS SrvSvcCopyNetSrvInfo(uint32 level, srvsvc_NetSrvInfo *info,
                                    uint8 **bufptr);

NET_API_STATUS SrvSvcCopyTIME_OF_DAY_INFO(PTIME_OF_DAY_INFO info,
                                          uint8 **bufptr);

#endif /* _SRVSVC_MEMORY_H_ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
