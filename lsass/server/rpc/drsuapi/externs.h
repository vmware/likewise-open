/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
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

/*
 * Abstract: Drsuapi interface (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#ifndef _EXTERNS_H_
#define _EXTERNS_H_

/* Library initialisation guard */
extern pthread_mutex_t gDrsuapiSrvDataMutex;

extern int bDrsuapiSrvInitialised;

extern PCSTR gpszDrsuapiRpcSrvName;
extern LSA_RPCSRV_FUNCTION_TABLE gDrsuapiRpcFuncTable;

extern rpc_binding_vector_p_t gpDrsuapiSrvBinding;

extern DRSUAPI_SRV_CONFIG gDrsuapiSrvConfig;

extern PSECURITY_DESCRIPTOR_ABSOLUTE gpDrsuapiSecDesc;

extern PLW_MAP_SECURITY_CONTEXT gpLsaSecCtx;

extern PHANDLE ghDirectory;

#if 0
extern DRSUAPI_GLOBALS gDrsuapiGlobals;
#endif



#endif /* _EXTERNS_H_ */
