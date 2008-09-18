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

#ifndef __TEST_H__
#define __TEST_H__

#define DAEMON_NAME "ctlibtest"

#if 1
#define PID_DIR "/tmp"
#else
#define PID_DIR "/var/run"
#endif

#define SYSLOG_ID DAEMON_NAME
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"
#define SERVER_PATH "/tmp/." DAEMON_NAME ".server"
#define CLIENT_PREFIX "/tmp/." DAEMON_NAME ".client"


#define TEST_MSG_VERSION 17

#define TEST_MSG_TYPE_HELLO 1
#define TEST_MSG_TYPE_HELLO_REPLY 2

#define ANY_SIZE 1

typedef struct _TEST_MSG_HELLO {
    uint32_t NameSize;
    uint32_t FromSize;
    char Name[ANY_SIZE];
    // From
} TEST_MSG_HELLO;

typedef struct _TEST_MSG_HELLO_REPLY {
    bool IsFormal;
    uint32_t GreetingSize;
    char Greeting[ANY_SIZE];
} TEST_MSG_HELLO_REPLY;


#endif /* __TEST_H__ */
