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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#ifndef __CT_FILEUTILS_H__
#define __CT_FILEUTILS_H__

#include <ctstatus.h>
#include <sys/types.h>
#include <sys/stat.h>

CT_STATUS
CtFileStat(
    IN const char* Path,
    OUT struct stat* Info
    );

CT_STATUS
CtFileUnlink(
    IN const char* Path
    );

CT_STATUS
CtFileRemoveDirectory(
    IN const char* Path
    );

CT_STATUS
CtFileRemoveTree(
    IN const char* Path
    );

CT_STATUS
CtFileSetOwner(
    IN const char* Path,
    IN uid_t Uid,
    IN gid_t Gid
    );

CT_STATUS
CtFileSetMode(
    IN const char* Path,
    IN mode_t Mode
    );

CT_STATUS
CtFileSetOwnerMode(
    IN const char* Path,
    IN uid_t Uid,
    IN gid_t Gid,
    IN mode_t Mode
    );

CT_STATUS
CtFileGetOwnerMode(
    IN const char* Path,
    OUT OPTIONAL uid_t* Uid,
    OUT OPTIONAL gid_t* Gid,
    OUT OPTIONAL mode_t* Mode
    );

#endif /* __CTFILEUTILS_H__ */
