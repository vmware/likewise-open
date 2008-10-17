/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) global variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

LSAMESSAGEHANDLER gMessageHandlers[] =
{
 { LSA_ERROR,                      LsaServer, NULL                      },//0
 { LSA_Q_GROUP_BY_ID,              LsaClient, &LsaSrvIpcFindGroupById   },//1
 { LSA_R_GROUP_BY_ID,              LsaServer, NULL                      },//2
 { LSA_Q_GROUP_BY_NAME,            LsaClient, &LsaSrvIpcFindGroupByName },//3
 { LSA_R_GROUP_BY_NAME,            LsaServer, NULL                      },//4
 { LSA_Q_BEGIN_ENUM_GROUPS,        LsaClient, &LsaSrvIpcBeginEnumGroups },//5
 { LSA_R_BEGIN_ENUM_GROUPS,        LsaServer, NULL                      },//6
 { LSA_Q_ENUM_GROUPS,              LsaClient, &LsaSrvIpcEnumGroups      },//7
 { LSA_R_ENUM_GROUPS,              LsaServer, NULL                      },//8
 { LSA_Q_END_ENUM_GROUPS,          LsaClient, &LsaSrvIpcEndEnumGroups   },//9
 { LSA_R_END_ENUM_GROUPS,          LsaServer, NULL                      },//10
 { LSA_Q_USER_BY_ID,               LsaClient, &LsaSrvIpcFindUserById    },//11
 { LSA_R_USER_BY_ID,               LsaServer, NULL                      },//12
 { LSA_Q_USER_BY_NAME,             LsaClient, &LsaSrvIpcFindUserByName  },//13
 { LSA_R_USER_BY_NAME,             LsaServer, NULL                      },//14
 { LSA_Q_BEGIN_ENUM_USERS,         LsaClient, &LsaSrvIpcBeginEnumUsers  },//15
 { LSA_R_BEGIN_ENUM_USERS,         LsaServer, NULL                      },//16
 { LSA_Q_ENUM_USERS,               LsaClient, &LsaSrvIpcEnumUsers       },//17
 { LSA_R_ENUM_USERS,               LsaServer, NULL                      },//18
 { LSA_Q_END_ENUM_USERS,           LsaClient, &LsaSrvIpcEndEnumUsers    },//19
 { LSA_R_END_ENUM_USERS,           LsaServer, NULL                      },//20
 { LSA_Q_AUTH_USER,                LsaClient, &LsaSrvIpcAuthenticateUser},//21
 { LSA_R_AUTH_USER,                LsaServer, NULL                      },//22
 { LSA_Q_VALIDATE_USER,            LsaClient, &LsaSrvIpcValidateUser    },//23
 { LSA_R_VALIDATE_USER,            LsaServer, NULL                      },//24
 { LSA_Q_ADD_USER,                 LsaClient, &LsaSrvIpcAddUser         },//25
 { LSA_R_ADD_USER,                 LsaServer, NULL                      },//26
 { LSA_Q_DELETE_USER,              LsaClient, &LsaSrvIpcDeleteUser      },//27
 { LSA_R_DELETE_USER,              LsaServer, NULL                      },//28
 { LSA_Q_ADD_GROUP,                LsaClient, &LsaSrvIpcAddGroup        },//29
 { LSA_R_ADD_GROUP,                LsaServer, NULL                      },//30
 { LSA_Q_DELETE_GROUP,             LsaClient, &LsaSrvIpcDeleteGroup     },//31
 { LSA_R_DELETE_GROUP,             LsaServer, NULL                      },//32
 { LSA_Q_CHANGE_PASSWORD,          LsaClient, &LsaSrvIpcChangePassword  },//33
 { LSA_R_CHANGE_PASSWORD,          LsaServer, NULL                      },//34
 { LSA_Q_OPEN_SESSION,             LsaClient, &LsaSrvIpcOpenSession     },//35
 { LSA_R_OPEN_SESSION,             LsaServer, NULL                      },//36
 { LSA_Q_CLOSE_SESSION,            LsaClient, &LsaSrvIpcCloseSession    },//37
 { LSA_R_CLOSE_SESSION,            LsaServer, NULL                      },//38
 { LSA_Q_MODIFY_USER,              LsaClient, &LsaSrvIpcModifyUser      },//39
 { LSA_R_MODIFY_USER,              LsaServer, NULL                      },//40
 { LSA_Q_GROUPS_FOR_USER,          LsaClient, &LsaSrvIpcGetGroupsForUser},//41
 { LSA_R_GROUPS_FOR_USER,          LsaServer, NULL                      },//42
 { LSA_Q_NAMES_BY_SID_LIST,        LsaClient, &LsaSrvIpcGetNamesBySidList},//43
 { LSA_R_NAMES_BY_SID_LIST,        LsaServer, NULL                      },//44
 { LSA_Q_GSS_MAKE_AUTH_MSG,        LsaClient, &LsaSrvIpcBuildAuthMessage},//45
 { LSA_R_GSS_MAKE_AUTH_MSG,        LsaServer, NULL                      },//46
 { LSA_Q_GSS_CHECK_AUTH_MSG,       LsaClient, &LsaSrvIpcCheckAuthMessage},//47
 { LSA_R_GSS_CHECK_AUTH_MSG,       LsaServer, NULL                      },//48
 { LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS, LsaClient, &LsaSrvIpcBeginEnumNSSArtefacts },//49
 { LSA_R_BEGIN_ENUM_NSS_ARTEFACTS, LsaServer, NULL                            },//50
 { LSA_Q_ENUM_NSS_ARTEFACTS,       LsaClient, &LsaSrvIpcEnumNSSArtefacts      },//51
 { LSA_R_ENUM_NSS_ARTEFACTS,       LsaServer, NULL                            },//52
 { LSA_Q_END_ENUM_NSS_ARTEFACTS,   LsaClient, &LsaSrvIpcEndEnumNSSArtefacts   },//53
 { LSA_R_END_ENUM_NSS_ARTEFACTS,   LsaServer, NULL                            },//54
 { LSA_Q_SET_LOGINFO,              LsaClient, &LsaSrvIpcSetLogInfo            },//55
 { LSA_R_SET_LOGINFO,              LsaServer, NULL                            },//56
 { LSA_Q_GET_LOGINFO,              LsaClient, &LsaSrvIpcGetLogInfo            },//57
 { LSA_R_GET_LOGINFO,              LsaServer, NULL                            },//58
 { LSA_Q_GET_METRICS,              LsaClient, &LsaSrvIpcGetMetrics            },//59
 { LSA_R_GET_METRICS,              LsaServer, NULL                            },//60
 { LSA_Q_GET_STATUS,               LsaClient, &LsaSrvIpcGetStatus             },//61
 { LSA_R_GET_STATUS,               LsaServer, NULL                            },//62
 { LSA_Q_REFRESH_CONFIGURATION,    LsaClient, &LsaSrvIpcRefreshConfiguration  },//63
 { LSA_R_REFRESH_CONFIGURATION,    LsaServer, NULL                            },//64
 { LSA_Q_CHECK_USER_IN_LIST,       LsaClient, &LsaSrvIpcCheckUserInList       },//65
 { LSA_R_CHECK_USER_IN_LIST,       LsaServer, NULL                            },//66
};
