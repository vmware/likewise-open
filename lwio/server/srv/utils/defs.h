/*
 * Copyright Likewise Software    2004-2009
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define REG_KEY_PATH_SRV_LOGGING_W \
              {'s','e','r','v','i','c','e','s','\\',         \
               'l','w','i','o','\\',                         \
               'p','a','r','a','m','e','t','e','r','s','\\', \
               'd','r','i','v','e','r','s','\\',             \
               's','r','v','\\',                             \
               'l','o','g','g','i','n','g',0}

#define REG_VALUE_SRV_LOGGING_ENABLED_W \
            {'E','n','a','b','l','e','L','o','g','g','i','n','g',0}

#define REG_VALUE_SRV_FILTERS_W \
            {'F','i','l','t','e','r','s',0}

typedef enum
{
    SRV_LOG_FILTER_TOKEN_TYPE_STAR = 0,
    SRV_LOG_FILTER_TOKEN_TYPE_OPEN_BRACE,
    SRV_LOG_FILTER_TOKEN_TYPE_CLOSE_BRACE,
    SRV_LOG_FILTER_TOKEN_TYPE_COMMA,
    SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT,
    SRV_LOG_FILTER_TOKEN_TYPE_EOF
} SRV_LOG_FILTER_TOKEN_TYPE;
