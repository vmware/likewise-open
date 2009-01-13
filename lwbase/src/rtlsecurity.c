
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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#include "includes.h"

NTSTATUS
RtlSetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN PSID  Group  OPTIONAL,
    IN BOOLEAN  GroupDefaulted
    )
{

    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PSID  *Group,
    OUT PBOOLEAN  GroupDefaulted
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlSetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN PSID  Owner  OPTIONAL,
    IN BOOLEAN  OwnerDefaulted
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PSID  *Owner,
    OUT PBOOLEAN  OwnerDefaulted
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlSetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN BOOLEAN  DaclPresent,
    IN PACL  Dacl  OPTIONAL,
    IN BOOLEAN  DaclDefaulted  OPTIONAL
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);

}
NTSTATUS
  RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PBOOLEAN  DaclPresent,
    OUT PACL  *Dacl,
    OUT PBOOLEAN  DaclDefaulted
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN BOOLEAN  SaclPresent,
    IN PACL  Sacl  OPTIONAL,
    IN BOOLEAN  SaclDefaulted  OPTIONAL
    )
{
    NTSTATUS ntStatus = 0;


    return(dwError);
}
NTSTATUS
RtlGetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PBOOLEAN  SaclPresent,
    OUT PACL  *Sacl,
    OUT PBOOLEAN  SaclDefaulted
    )
{
    NTSTATUS ntStatus = 0;

    return(dwError);
}

NTSTATUS
  RtlCreateAcl(
    IN PACL  Acl,
    IN ULONG  AclLength,
    IN ULONG  AclRevision
    )
{
    NTSTATUS ntStatus = 0;


    return(ntStatus);
}

DWORD
RtlAddAce()
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
RtlGetAce()
{
    NTSTATUS ntStatus = 0;



    return(ntStatus);

}

DWORD
RtlDeleteAce(
    IN OUT PACL  Acl,
    IN ULONG  AceIndex
    )
{

    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

BOOLEAN
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR  SecurityDescriptorInput,
    IN ULONG  SecurityDescriptorLength,
    IN SECURITY_INFORMATION  RequiredInformation
    )
{
    BOOLEAN bRet = FALSE;

    return(bRet);
}

DWORD
RtlCreateSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN ULONG  Revision
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

ULONG
RtlLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor
    )
{
    NTSTATUS ntStatus = 0;


    return(ntStatus);
}


NTSTATUS
RtlAccessCheck(
   IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
   IN HANDLE hClientToken,
   IN DWORD DesiredAccess,
   PGENERIC_MAPPING GenericMapping,
   PPRIVILEGE_SET PrivilegeSet,
   PDWORD PrivilegeSetLength,
   PDWORD GrantedAccess
   )
{
    NTSTATUS ntStatus= 0;

    return(ntStatus);
}

