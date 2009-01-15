
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

DWORD
SetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN PSID  Group  OPTIONAL,
    IN BOOLEAN  GroupDefaulted
    )
{
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlSetGroupSecurityDescriptor(
                        SecurityDescriptor,
                        Group,
                        GroupDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);

    return(dwError);
}

NTSTATUS
GetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PSID  *Group,
    OUT PBOOLEAN  GroupDefaulted
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlGetGroupSecurityDescriptor(
                        SecurityDescriptor,
                        Group,
                        GroupDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);

    return(dwError);
}

DWORD
SetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN PSID  Owner  OPTIONAL,
    IN BOOLEAN  OwnerDefaulted
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlSetOwnerSecurityDescriptor(
                        SecurityDescriptor,
                        Owner,
                        OwnerDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);

    return(dwError);
}

NTSTATUS
GetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PSID  *Owner,
    OUT PBOOLEAN  OwnerDefaulted
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;

    ntStatus = RtlGetOwnerSecurityDescriptor(
                        SecurityDescriptor,
                        Owner,
                        OwnerDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);


    return(dwError);
}

DWORD
SetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN BOOLEAN  DaclPresent,
    IN PACL  Dacl  OPTIONAL,
    IN BOOLEAN  DaclDefaulted  OPTIONAL
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlSetDaclSecurityDescriptor(
                    SecurityDescriptor,
                    DaclPresent,
                    Dacl,
                    DaclDefaulted
                    );

    dwError = MapNTStatustoWin32(ntStatus);


    return(dwError);

}

DWORD
GetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PBOOLEAN  DaclPresent,
    OUT PACL  *Dacl,
    OUT PBOOLEAN  DaclDefaulted
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;


    ntStatus = RtlGetDaclSecurityDescriptor(
                        SecurityDescriptor,
                        DaclPresent,
                        Dacl,
                        DaclDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);


    return(dwError);
}

DWORD
SetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN BOOLEAN  SaclPresent,
    IN PACL  Sacl  OPTIONAL,
    IN BOOLEAN  SaclDefaulted  OPTIONAL
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;

    ntStatus = RtlSetSaclSecurityDescriptor(
                        SecurityDescriptor,
                        SaclPresent,
                        Sacl,
                        SaclDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);

    return(dwError);
}

DWORD
GetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
    OUT PBOOLEAN  SaclPresent,
    OUT PACL  *Sacl,
    OUT PBOOLEAN  SaclDefaulted
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;


    ntStatus = RtlGetSaclSecurityDescriptor(
                        SecurityDescriptor,
                        SaclPresent,
                        Sacl,
                        SaclDefaulted
                        );
    dwError = MapNTStatustoWin32(ntStatus);

    return(dwError);
}

DWORD
CreateAcl(
    IN PACL  Acl,
    IN ULONG  AclLength,
    IN ULONG  AclRevision
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlCreateAcl(Acl, AclLength, AclRevision);

    dwError = MapNTStatustoWin32(ntStatus);


    return(ntStatus);
}

DWORD
AddAce()
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;


    ntStatus = RtlSetGroupSecurityDescriptor(
                        );
    dwError = MapNTStatustoWin32(ntStatus);


    return(ntStatus);
}

NTSTATUS
GetAce()
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlSetGroupSecurityDescriptor(
                        );
    dwError = MapNTStatustoWin32(ntStatus);


    return(ntStatus);

}

DWORD
DeleteAce(
    IN OUT PACL  Acl,
    IN ULONG  AceIndex
    )
{

    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;
    NTSTATUS ntStatus = 0;

    ntStatus = RtlDeleteAce(Acl, AceIndex);
    dwError = MapNTStatustoWin32(ntStatus);

    return(ntStatus);

}

BOOLEAN
IsValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR  SecurityDescriptorInput,
    IN ULONG  SecurityDescriptorLength,
    IN SECURITY_INFORMATION  RequiredInformation
    )
{
    BOOLEAN bRet = FALSE;
    DWORD    dwError = 0;

    ntStatus = RtlValidRelativeSecurityDescriptor(
                        SecurityDescriptorInput,
                        SecurityDescriptorLength,
                        RequiredInformation
                        );
    dwError = MapNTStatustoWin32(ntStatus);


    return(bRet);
}

DWORD
CreateSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN ULONG  Revision
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;

    ntStatus = RtlCreateSecurityDescriptor(
                    SecurityDescriptor,
                    Revision
                    );
    dwError = MapNTStatustoWin32(ntStatus);


    return(ntStatus);
}

ULONG
GetLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor
    )
{
    NTSTATUS ntStatus = 0;
    DWORD    dwError = 0;


    ntStatus = RtlLengthSecurityDescriptor(SecurityDescriptor);

    dwError = MapNTStatustoWin32(ntStatus);

    return(ntStatus);
}
