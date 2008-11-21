/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaipc.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSAIPC_H__
#define __LSAIPC_H__

#define LSA_CLIENT_PATH_FORMAT "/var/tmp/.lsaclient_%05ld"
#define LSA_SERVER_FILENAME    ".lsasd"

typedef enum {
    LSA_ERROR                      =   0,
    LSA_Q_GROUP_BY_ID              =   1,
    LSA_R_GROUP_BY_ID              =   2,
    LSA_Q_GROUP_BY_NAME            =   3,
    LSA_R_GROUP_BY_NAME            =   4,
    LSA_Q_BEGIN_ENUM_GROUPS        =   5,
    LSA_R_BEGIN_ENUM_GROUPS        =   6,
    LSA_Q_ENUM_GROUPS              =   7,
    LSA_R_ENUM_GROUPS              =   8,
    LSA_Q_END_ENUM_GROUPS          =   9,
    LSA_R_END_ENUM_GROUPS          =  10,
    LSA_Q_USER_BY_ID               =  11,
    LSA_R_USER_BY_ID               =  12,
    LSA_Q_USER_BY_NAME             =  13,
    LSA_R_USER_BY_NAME             =  14,
    LSA_Q_BEGIN_ENUM_USERS         =  15,
    LSA_R_BEGIN_ENUM_USERS         =  16,
    LSA_Q_ENUM_USERS               =  17,
    LSA_R_ENUM_USERS               =  18,
    LSA_Q_END_ENUM_USERS           =  19,
    LSA_R_END_ENUM_USERS           =  20,
    LSA_Q_AUTH_USER                =  21,
    LSA_R_AUTH_USER                =  22,
    LSA_Q_VALIDATE_USER            =  23,
    LSA_R_VALIDATE_USER            =  24,
    LSA_Q_ADD_USER                 =  25,
    LSA_R_ADD_USER                 =  26,
    LSA_Q_DELETE_USER              =  27,
    LSA_R_DELETE_USER              =  28,
    LSA_Q_ADD_GROUP                =  29,
    LSA_R_ADD_GROUP                =  30,
    LSA_Q_DELETE_GROUP             =  31,
    LSA_R_DELETE_GROUP             =  32,
    LSA_Q_CHANGE_PASSWORD          =  33,
    LSA_R_CHANGE_PASSWORD          =  34,
    LSA_Q_OPEN_SESSION             =  35,
    LSA_R_OPEN_SESSION             =  36,
    LSA_Q_CLOSE_SESSION            =  37,
    LSA_R_CLOSE_SESSION            =  38,
    LSA_Q_MODIFY_USER              =  39,
    LSA_R_MODIFY_USER              =  40,
    LSA_Q_GROUPS_FOR_USER          =  41,
    LSA_R_GROUPS_FOR_USER          =  42,
    LSA_Q_NAMES_BY_SID_LIST        =  43,
    LSA_R_NAMES_BY_SID_LIST        =  44,
    LSA_Q_GSS_MAKE_AUTH_MSG        =  45,
    LSA_R_GSS_MAKE_AUTH_MSG        =  46,
    LSA_Q_GSS_CHECK_AUTH_MSG       =  47,
    LSA_R_GSS_CHECK_AUTH_MSG       =  48,
    LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS =  49,
    LSA_R_BEGIN_ENUM_NSS_ARTEFACTS =  50,
    LSA_Q_ENUM_NSS_ARTEFACTS       =  51,
    LSA_R_ENUM_NSS_ARTEFACTS       =  52,
    LSA_Q_END_ENUM_NSS_ARTEFACTS   =  53,
    LSA_R_END_ENUM_NSS_ARTEFACTS   =  54,
    LSA_Q_SET_LOGINFO              =  55,
    LSA_R_SET_LOGINFO              =  56,
    LSA_Q_GET_LOGINFO              =  57,
    LSA_R_GET_LOGINFO              =  58,
    LSA_Q_GET_METRICS              =  59,
    LSA_R_GET_METRICS              =  60,
    LSA_Q_GET_STATUS               =  61,
    LSA_R_GET_STATUS               =  62,
    LSA_Q_REFRESH_CONFIGURATION    =  63,
    LSA_R_REFRESH_CONFIGURATION    =  64,
    LSA_Q_CHECK_USER_IN_LIST       =  65,
    LSA_R_CHECK_USER_IN_LIST       =  66,
    LSA_Q_FIND_NSS_ARTEFACT_BY_KEY =  67,
    LSA_R_FIND_NSS_ARTEFACT_BY_KEY =  68,
    LSA_Q_AUTH_USER_EX             =  69,
    LSA_R_AUTH_USER_EX             =  70,
    LSA_Q_SET_TRACE_INFO           =  71,
    LSA_R_SET_TRACE_INFO           =  72,
    LSA_Q_GET_TRACE_INFO           =  73,
    LSA_R_GET_TRACE_INFO           =  74,
    LSA_Q_ENUM_TRACE_INFO          =  75,
    LSA_R_ENUM_TRACE_INFO          =  76,
    LSA_MESSAGE_SENTINEL
} LsaMessageType;

typedef struct LsaMessageHeaderTag {
    /* type of group policy message */
    uint8_t   messageType;
    /* protocol version */
    uint8_t   version;
    /* This may be used for sequencing
     * For instance, 1 of 10, 2 of 10
     */
    uint16_t  reserved[2];
    /* The length of the attached message
     * This is in network format
     */
    uint32_t  messageLength;
} LSAMESSAGEHEADER, *PLSAMESSAGEHEADER;

typedef struct LsaMessageTag {
    LSAMESSAGEHEADER header;
    PSTR pData;
} LSAMESSAGE, *PLSAMESSAGE;

#define LSA_SAFE_FREE_MESSAGE(pMessage) \
    if (pMessage) {                     \
        LsaFreeMessage(pMessage);       \
        pMessage = NULL;                \
    }

typedef DWORD (*PFNMESSAGESCREENER) (PLSAMESSAGE pMessage, uid_t peerUID);

typedef struct __LSA_ENUM_RECORD_QUERY_HEADER {
    DWORD dwInfoLevel;
    DWORD dwStartingRecordId;
    DWORD dwMaxNumRecords;
    LSADATACOORDINATES domain;
} LSA_ENUM_RECORD_QUERY_HEADER, *PLSA_ENUM_RECORD_QUERY_HEADER;

typedef struct __LSA_QUERY_RECORD_BY_NAME_HEADER {
    LSA_FIND_FLAGS FindFlags;
    DWORD dwInfoLevel;
    LSADATACOORDINATES name;
} LSA_QUERY_RECORD_BY_NAME_HEADER, *PLSA_QUERY_RECORD_BY_NAME_HEADER;

typedef struct __LSA_QUERY_RECORD_BY_ID_HEADER {
    LSA_FIND_FLAGS FindFlags;
    DWORD dwInfoLevel;
    DWORD id;
} LSA_QUERY_RECORD_BY_ID_HEADER, *PLSA_QUERY_RECORD_BY_ID_HEADER;

typedef struct __LSA_USER_GROUP_RECORD_PREAMBLE {
    DWORD dwNumRecords;
    DWORD dwInfoLevel;
} LSA_USER_GROUP_RECORD_PREAMBLE, *PLSA_USER_GROUP_RECORD_PREAMBLE;

typedef struct __LSA_USER_NSS_ARTEFACT_RECORD_PREAMBLE {
    DWORD dwNumRecords;
    DWORD dwInfoLevel;
} LSA_USER_NSS_ARTEFACT_RECORD_PREAMBLE, *PLSA_NSS_ARTEFACT_GROUP_RECORD_PREAMBLE;

typedef struct __LSA_QUERY_GET_NAMES_BY_SID_LIST {
    DWORD dwCount;
    LSADATACOORDINATES coordinates[1];
} LSA_QUERY_GET_NAMES_BY_SID_LIST, *PLSA_QUERY_GET_NAMES_BY_SID_LIST;

typedef struct __LSA_REPLY_GET_NAMES_BY_SID_LIST {
    DWORD dwCount;
    CHAR chDomainSeparator;
    struct
    {
        LSADATACOORDINATES domainName;
        LSADATACOORDINATES samAccount;
        DWORD dwType; //casted ADAccountType
    } entries[1];
} LSA_REPLY_GET_NAMES_BY_SID_LIST, *PLSA_REPLY_GET_NAMES_BY_SID_LIST;

typedef struct __LSA_USER_0_RECORD_HEADER {
    DWORD uid;
    DWORD gid;
    LSADATACOORDINATES name;
    LSADATACOORDINATES passwd;
    LSADATACOORDINATES gecos;
    LSADATACOORDINATES homedir;
    LSADATACOORDINATES shell;
    LSADATACOORDINATES sid;
} LSA_USER_0_RECORD_HEADER, *PLSA_USER_0_RECORD_HEADER;

typedef struct __LSA_USER_1_RECORD_HEADER {
    LSA_USER_0_RECORD_HEADER record0;
    DWORD bIsLocalUser;
    DWORD bIsGeneratedUPN;
    LSADATACOORDINATES upn;
    LSADATACOORDINATES NTHash;
    LSADATACOORDINATES LMHash;
} LSA_USER_1_RECORD_HEADER, *PLSA_USER_1_RECORD_HEADER;

typedef struct __LSA_USER_2_RECORD_HEADER {
    LSA_USER_1_RECORD_HEADER record1;
    DWORD dwDaysToPasswordExpiry;
    DWORD dwAccountExpired;
    DWORD dwPasswordExpired;
    DWORD dwPasswordNeverExpires;
    DWORD dwPromptChangePassword;
    DWORD dwUserCanChangePassword;
    DWORD dwAccountLocked;
    DWORD dwAccountDisabled;
} LSA_USER_2_RECORD_HEADER, *PLSA_USER_2_RECORD_HEADER;

typedef struct __LSA_MOD_USER_RECORD_HEADER
{
    DWORD uid;

    struct {
      WORD bEnableUser;
      WORD bDisableUser;
      WORD bUnlockUser;
      WORD bChangePasswordOnNextLogon;
      WORD bSetPasswordNeverExpires;
      WORD bSetPasswordMustExpire;
      WORD bAddToGroups;
      WORD bRemoveFromGroups;
      WORD bSetAccountExpiryDate;
    } actions;

    LSADATACOORDINATES addToGroups;
    LSADATACOORDINATES removeFromGroups;
    LSADATACOORDINATES accountExpiryDate;
} LSA_MOD_USER_RECORD_HEADER, *PLSA_MOD_USER_RECORD_HEADER;

typedef struct __LSA_GROUP_0_RECORD_HEADER {
    DWORD gid;
    LSADATACOORDINATES name;
    LSADATACOORDINATES sid;
} LSA_GROUP_0_RECORD_HEADER, *PLSA_GROUP_0_RECORD_HEADER;

typedef struct __LSA_GROUP_1_RECORD_HEADER {
    DWORD gid;
    LSADATACOORDINATES name;
    LSADATACOORDINATES passwd;
    LSADATACOORDINATES sid;
    LSADATACOORDINATES gr_mem;
} LSA_GROUP_1_RECORD_HEADER, *PLSA_GROUP_1_RECORD_HEADER;

typedef struct __LSA_BEGIN_ENUM_RECORDS_HEADER {
    DWORD dwInfoLevel;
    DWORD dwNumMaxRecords;
} LSA_BEGIN_ENUM_RECORDS_HEADER, *PLSA_BEGIN_ENUM_RECORDS_HEADER;

typedef struct __LSA_BEGIN_ENUM_NSS_ARTEFACT_RECORDS_HEADER {
    DWORD dwInfoLevel;
    DWORD dwNumMaxRecords;
    LSA_NIS_MAP_QUERY_FLAGS dwFlags;
    LSADATACOORDINATES mapName;
} LSA_BEGIN_ENUM_NSS_ARTEFACT_RECORDS_HEADER, *PLSA_BEGIN_ENUM_NSS_ARTEFACT_RECORDS_HEADER;

typedef struct __LSA_FIND_NSS_ARTEFACT_BY_KEY_HEADER {
    DWORD dwInfoLevel;
    LSA_NIS_MAP_QUERY_FLAGS dwFlags;
    LSADATACOORDINATES mapName;
    LSADATACOORDINATES keyName;
} LSA_FIND_NSS_ARTEFACT_BY_KEY_HEADER, *PLSA_FIND_NSS_ARTEFACT_BY_KEY_HEADER;

typedef struct __LSA_NSS_ARTEFACT_0_RECORD_HEADER {
    DWORD dwMapType;
    LSADATACOORDINATES name;
    LSADATACOORDINATES value;
} LSA_NSS_ARTEFACT_0_RECORD_HEADER, *PLSA_NSS_ARTEFACT_0_RECORD_HEADER;

typedef struct __LSA_ENUM_RECORDS_TOKEN_HEADER {
    LSADATACOORDINATES token;
} LSA_ENUM_RECORDS_TOKEN_HEADER, *PLSA_ENUM_RECORDS_TOKEN_HEADER;

/*
 * GSS specific messages go here
 */

typedef struct _LSA_GSS_Q_MAKE_AUTH_MSG {
    ULONG negotiateFlags;
    LSADATACOORDINATES marshalledAuthUser;
    LSADATACOORDINATES serverChallenge;
    LSADATACOORDINATES targetInfo;
} LSA_GSS_Q_MAKE_AUTH_MSG, *PLSA_GSS_Q_MAKE_AUTH_MSG;

typedef struct _LSA_GSS_R_MAKE_AUTH_MSG {
    DWORD msgError;
    LSADATACOORDINATES authenticateMessage;
    LSADATACOORDINATES baseSessionKey;
} LSA_GSS_R_MAKE_AUTH_MSG, *PLSA_GSS_R_MAKE_AUTH_MSG;

typedef struct _LSA_GSS_Q_CHECK_AUTH_MSG {
    ULONG negotiateFlags;
    LSADATACOORDINATES serverChallenge;
    LSADATACOORDINATES targetInfo;
    LSADATACOORDINATES authenticateMessage;
} LSA_GSS_Q_CHECK_AUTH_MSG, *PLSA_GSS_Q_CHECK_AUTH_MSG;

typedef struct _LSA_GSS_R_CHECK_AUTH_MSG {
    DWORD msgError;
    LSADATACOORDINATES baseSessionKey;
    /* other data - user info? here */
} LSA_GSS_R_CHECK_AUTH_MSG, *PLSA_GSS_R_CHECK_AUTH_MSG;

/* end GSS */

/* Logging */

typedef struct __LSA_LOG_INFO_MSG {
    LsaLogLevel        maxAllowedLogLevel;
    LsaLogTarget       logTarget;
    LSADATACOORDINATES path;
} LSA_LOG_INFO_MSG, *PLSA_LOG_INFO_MSG;

/* Metrics */

typedef struct __LSA_METRICS_HEADER
{
    // LSADATACOORDINATES id;
    DWORD dwInfoLevel;

} LSA_METRICS_HEADER, *PLSA_METRICS_HEADER;

typedef struct __LSA_DC_INFO_MSG {
    LSADATACOORDINATES name;
    LSADATACOORDINATES address;
    LSADATACOORDINATES siteName;
    LSA_DS_FLAGS       dwFlags;
} LSA_DC_INFO_MSG, *PLSA_DC_INFO_MSG;

typedef struct __LSA_DOMAIN_INFO_MSG {

    LSADATACOORDINATES  dnsDomain;
    LSADATACOORDINATES  netbiosDomain;
    LSADATACOORDINATES  trusteeDnsDomain;
    LSADATACOORDINATES  domainSID;
    LSADATACOORDINATES  domainGUID;
    LSADATACOORDINATES  forestName;
    LSADATACOORDINATES  clientSiteName;
    LSA_TRUST_FLAG      dwTrustFlags;
    LSA_TRUST_TYPE      dwTrustType;
    LSA_TRUST_ATTRIBUTE dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE      dwTrustMode;
    LSA_DM_DOMAIN_FLAGS dwDomainFlags;
    DWORD               dwDCInfoOffset;
    DWORD               dwGCInfoOffset;

} LSA_DOMAIN_INFO_MSG, *PLSA_DOMAIN_INFO_MSG;

typedef struct __LSA_AUTH_PROVIDER_STATUS_MSG {

      DWORD dwMode;
      DWORD dwSubmode;
      DWORD dwStatus;
      DWORD dwNetworkCheckInterval;
      DWORD dwNumTrustedDomains;
      LSADATACOORDINATES id;
      LSADATACOORDINATES domain;
      LSADATACOORDINATES forest;
      LSADATACOORDINATES site;
      LSADATACOORDINATES cell;
      DWORD dwTrustedDomainOffset;
} LSA_AUTH_PROVIDER_STATUS_MSG, *PLSA_AUTH_PROVIDER_STATUS_MSG;

typedef struct __LSA_STATUS_MSG {

      DWORD       dwUptime;
      LSA_VERSION version;
      DWORD       dwCount;

} LSA_STATUS_MSG, *PLSA_STATUS_MSG;

DWORD
LsaOpenServer(
    PHANDLE phConnection
    );

DWORD
LsaCloseServer(
    HANDLE hConnection
    );

DWORD
LsaMarshalBeginEnumRecordsQuery(
    DWORD  dwInfoLevel,
    DWORD  dwNumMaxRecords,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalBeginEnumRecordsQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    PDWORD pdwNumMaxRecords
    );

DWORD
LsaMarshalEnumRecordsToken(
    PCSTR  pszGUID,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalEnumRecordsToken(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PSTR*  ppszGUID
    );

DWORD
LsaMarshalFindUserByNameQuery(
    PCSTR  pszLoginId,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalFindUserByNameQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszLoginId,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalFindUserByIdQuery(
    uid_t  uid,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalFindUserByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    uid_t* pUid,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalDeleteUserByIdQuery(
    uid_t uid,
    PSTR  pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalDeleteUserByIdQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    uid_t* pUid
    );

DWORD
LsaMarshalFindGroupByNameQuery(
    PCSTR  pszGroupName,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalFindGroupByNameQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszGroupName,
    PLSA_FIND_FLAGS pFindFlags,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalFindGroupByIdQuery(
    gid_t  gid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalFindGroupByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    gid_t* pGid,
    PLSA_FIND_FLAGS pFindFlags,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalDeleteGroupByIdQuery(
    gid_t  gid,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalDeleteGroupByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    gid_t* pGid
    );

DWORD
LsaMarshalGetGroupsForUserQuery(
    uid_t  uid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalGetGroupsForUserQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    uid_t* pUid,
    PLSA_FIND_FLAGS pFindFlags,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalUserInfoList(
    PVOID* ppUserInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumUsers,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalUserInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsers
    );

DWORD
LsaMarshalGroupInfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumGroups,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalGroupInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroups
    );


DWORD
LsaMarshalNSSArtefactInfoList(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumNSSArtefacts,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalNSSArtefactInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefacts
    );

DWORD
LsaMarshalBeginEnumNSSArtefactRecordsQuery(
    DWORD  dwInfoLevel,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwNumMaxRecords,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalBeginEnumNSSArtefactRecordsQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS* pdwFlags,
    PSTR*  ppszMapName,
    PDWORD pdwNumMaxRecords
    );

DWORD
LsaMarshalFindNSSArtefactByKeyQuery(
    DWORD  dwInfoLevel,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PSTR   pszBuffer,
    PDWORD pdwMsgLen
    );

DWORD
LsaUnmarshalFindNSSArtefactByKeyQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS* pdwFlags,
    PSTR*  ppszMapName,
    PSTR*  ppszKeyName
    );

DWORD
LsaMarshalUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalUserModInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

DWORD
LsaMarshalLogInfo(
    LsaLogLevel logLevel,
    LsaLogTarget logTarget,
    PCSTR        pszPath,
    PSTR         pszBuffer,
    PDWORD       pdwBufLen
    );

DWORD
LsaUnmarshalLogInfo(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LsaMarshalTraceFlags(
    PLSA_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags,
    PSTR            pszBuffer,
    PDWORD          pdwMsgLen
    );

DWORD
LsaUnmarshalTraceFlags(
    PCSTR            pszBuffer,
    DWORD            dwMsgLen,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    );

DWORD
LsaMarshalQueryTraceFlag(
    DWORD           dwTraceFlag,
    PSTR            pszBuffer,
    PDWORD          pdwMsgLen
    );

DWORD
LsaUnmarshalQueryTraceFlag(
    PSTR            pszBuffer,
    DWORD           dwMsgLen,
    PDWORD          pdwTraceFlag
    );

/* Builds a message object with the data field allocated - but, not filled in */
DWORD
LsaBuildMessage(
    LsaMessageType msgType,
    uint32_t       msgLen,
    uint16_t       iData,
    uint16_t       nData,
    PLSAMESSAGE*   ppMessage
    );

void
LsaFreeMessage(
    PLSAMESSAGE pMessage
    );

DWORD
LsaReadNextMessage(
    int         fd,
    PLSAMESSAGE *ppMessage
    );

DWORD
LsaSecureReadNextMessage(
    int                fd,
    uid_t              peerUID,
    PFNMESSAGESCREENER pFnScreener,
    PLSAMESSAGE        *ppMessage
    );

DWORD
LsaWriteMessage(
    int   fd,
    const PLSAMESSAGE pMessage
    );

DWORD
LsaSendCreds(
    int fd
    );

DWORD
LsaRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid
    );

DWORD
LsaWriteData(
    DWORD dwFd,
    PSTR  pszBuf,
    DWORD dwLen);

DWORD
LsaReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead);

DWORD
LsaSendMsg(
    DWORD dwFd,
    const struct msghdr *pMsg
    );

DWORD
LsaRecvMsg(
    DWORD dwFd,
    struct msghdr *pMsg
    );

DWORD
LsaMarshalGetNamesBySidListQuery(
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR   pszBuffer,
    PDWORD pdwBufLen);

DWORD
LsaUnmarshalGetNamesBySidListQuery(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    size_t* psCount,
    PSTR**  pppszSidList
    );

DWORD
LsaMarshalGetNamesBySidListReply(
    IN size_t sCount,
    IN PSTR* ppszDomainNames,
    IN PSTR* ppszSamAccounts,
    IN ADAccountType* pTypes,
    IN CHAR chDomainSeparator,
    IN OUT PSTR pszBuffer,
    IN OUT PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalGetNamesBySidListReply(
    IN PCSTR pszMsgBuf,
    IN DWORD dwMsgLen,
    OUT size_t* psCount,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT CHAR* pchDomainSeparator
    );

DWORD
LsaMarshalMetricsInfo(
    DWORD  dwInfoLevel,
    PVOID  pMetricPack,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalMetricsInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    PVOID* ppMetricPack
    );

DWORD
LsaMarshalStatus(
    PLSASTATUS pLsaStatus,
    PSTR       pszBuffer,
    PDWORD     pdwBufLen
    );

DWORD
LsaUnmarshalStatus(
    PCSTR       pszMsgBuf,
    DWORD       dwMsgLen,
    PLSASTATUS* ppLsaStatus
    );


#endif /*__LSAIPC_H__*/
