/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        archive.h
 *
 * Abstract:
 *
 *        Archive file API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ARCHIVE_H__
#define __LWMSG_ARCHIVE_H__

#include <lwmsg/assoc.h>
#include <lwmsg/protocol.h>

#include <sys/types.h>

/**
 * @file archive.h
 * @brief Persistence API
 */

/**
 * @defgroup archive Persistent archives
 * @ingroup public
 * @brief Serialize messages to permanent storage
 *
 */

/*@{*/

typedef struct LWMsgArchive LWMsgArchive;

typedef enum LWMsgArchiveDisposition
{
    LWMSG_ARCHIVE_READ = 0x1,
    LWMSG_ARCHIVE_WRITE = 0x2
} LWMsgArchiveDisposition;

LWMsgStatus
lwmsg_archive_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgArchive** archive
    );

LWMsgStatus
lwmsg_archive_set_file(
    LWMsgArchive* archive,
    const char* filename,
    LWMsgArchiveDisposition disp,
    mode_t mode
    );

LWMsgStatus
lwmsg_archive_set_fd(
    LWMsgArchive* archive,
    int fd,
    LWMsgArchiveDisposition dist
    );

LWMsgStatus
lwmsg_archive_set_byte_order(
    LWMsgArchive* archive,
    LWMsgByteOrder order
    );

LWMsgStatus
lwmsg_archive_open(
    LWMsgArchive* archive
    );

LWMsgStatus
lwmsg_archive_close(
    LWMsgArchive* archive
    );

LWMsgStatus
lwmsg_archive_write_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

LWMsgStatus
lwmsg_archive_read_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

void
lwmsg_archive_delete(
    LWMsgArchive* archive
    );

LWMsgAssoc*
lwmsg_archive_as_assoc(
    LWMsgArchive* archive
    );

/*@}*/

#endif
