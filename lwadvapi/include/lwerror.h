/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __LW_ERROR_H__
#define __LW_ERROR_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <winerror.h>

#include <lwmsg/status.h>

#define BAIL_ON_LW_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            LW_LOG_DEBUG("Error at %s:%d [code: %d]", __FILE__, __LINE__, dwError); \
            goto error; \
        } \
    } while (0)

#define BAIL_ON_LDAP_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            dwError = LwMapLdapErrorToLwError(dwError); \
            LW_LOG_DEBUG("Error at %s:%d [code: %d]", __FILE__, __LINE__, dwError); \
            goto error; \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_STRING(pszParam) \
    do { \
        if (LW_IS_NULL_OR_EMPTY_STR(pszParam)) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_HANDLE(hParam) \
    do { \
        if (!hParam) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_POINTER(p) \
    do { \
        if (!p) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

/** Success */
#define LW_ERROR_SUCCESS                                   0

#define LW_ERROR_INVALID_CACHE_PATH                        40001
#define LW_ERROR_INVALID_CONFIG_PATH                       40002
#define LW_ERROR_INVALID_PREFIX_PATH                       40003
#define LW_ERROR_INSUFFICIENT_BUFFER                       40004
#define LW_ERROR_OUT_OF_MEMORY                             40005
#define LW_ERROR_INVALID_MESSAGE                           40006
#define LW_ERROR_UNEXPECTED_MESSAGE                        40007
#define LW_ERROR_NO_SUCH_USER                              40008
#define LW_ERROR_DATA_ERROR                                40009
#define LW_ERROR_NOT_IMPLEMENTED                           40010
#define LW_ERROR_NO_CONTEXT_ITEM                           40011
#define LW_ERROR_NO_SUCH_GROUP                             40012
#define LW_ERROR_REGEX_COMPILE_FAILED                      40013
#define LW_ERROR_NSS_EDIT_FAILED                           40014
#define LW_ERROR_NO_HANDLER                                40015
#define LW_ERROR_INTERNAL                                  40016
#define LW_ERROR_NOT_HANDLED                               40017
#define LW_ERROR_INVALID_DNS_RESPONSE                      40018
#define LW_ERROR_DNS_RESOLUTION_FAILED                     40019
#define LW_ERROR_FAILED_TIME_CONVERSION                    40020
#define LW_ERROR_INVALID_SID                               40021
#define LW_ERROR_PASSWORD_MISMATCH                         40022
#define LW_ERROR_UNEXPECTED_DB_RESULT                      40023
#define LW_ERROR_PASSWORD_EXPIRED                          40024
#define LW_ERROR_ACCOUNT_EXPIRED                           40025
#define LW_ERROR_USER_EXISTS                               40026
#define LW_ERROR_GROUP_EXISTS                              40027
#define LW_ERROR_INVALID_GROUP_INFO_LEVEL                  40028
#define LW_ERROR_INVALID_USER_INFO_LEVEL                   40029
#define LW_ERROR_UNSUPPORTED_USER_LEVEL                    40030
#define LW_ERROR_UNSUPPORTED_GROUP_LEVEL                   40031
#define LW_ERROR_INVALID_LOGIN_ID                          40032
#define LW_ERROR_INVALID_HOMEDIR                           40033
#define LW_ERROR_INVALID_GROUP_NAME                        40034
#define LW_ERROR_NO_MORE_GROUPS                            40035
#define LW_ERROR_NO_MORE_USERS                             40036
#define LW_ERROR_FAILED_ADD_USER                           40037
#define LW_ERROR_FAILED_ADD_GROUP                          40038
#define LW_ERROR_INVALID_LSA_CONNECTION                    40039
#define LW_ERROR_INVALID_AUTH_PROVIDER                     40040
#define LW_ERROR_INVALID_PARAMETER                         40041
#define LW_ERROR_LDAP_NO_PARENT_DN                         40042
#define LW_ERROR_LDAP_ERROR                                40043
#define LW_ERROR_NO_SUCH_DOMAIN                            40044
#define LW_ERROR_LDAP_FAILED_GETDN                         40045
#define LW_ERROR_DUPLICATE_DOMAINNAME                      40046
#define LW_ERROR_KRB5_CALL_FAILED                          40047
#define LW_ERROR_GSS_CALL_FAILED                           40048
#define LW_ERROR_FAILED_FIND_DC                            40049
#define LW_ERROR_NO_SUCH_CELL                              40050
#define LW_ERROR_GROUP_IN_USE                              40051
#define LW_ERROR_FAILED_CREATE_HOMEDIR                     40052
#define LW_ERROR_PASSWORD_TOO_WEAK                         40053
#define LW_ERROR_INVALID_SID_REVISION                      40054
#define LW_ERROR_ACCOUNT_LOCKED                            40055
#define LW_ERROR_ACCOUNT_DISABLED                          40056
#define LW_ERROR_USER_CANNOT_CHANGE_PASSWD                 40057
#define LW_ERROR_LOAD_LIBRARY_FAILED                       40058
#define LW_ERROR_LOOKUP_SYMBOL_FAILED                      40059
#define LW_ERROR_INVALID_EVENTLOG                          40060
#define LW_ERROR_INVALID_CONFIG                            40061
#define LW_ERROR_UNEXPECTED_TOKEN                          40062
#define LW_ERROR_LDAP_NO_RECORDS_FOUND                     40063
#define LW_ERROR_DUPLICATE_USERNAME                        40064
#define LW_ERROR_DUPLICATE_GROUPNAME                       40065
#define LW_ERROR_DUPLICATE_CELLNAME                        40066
#define LW_ERROR_STRING_CONV_FAILED                        40067
#define LW_ERROR_INVALID_ACCOUNT                           40068
#define LW_ERROR_INVALID_PASSWORD                          40069
#define LW_ERROR_QUERY_CREATION_FAILED                     40070
#define LW_ERROR_NO_SUCH_OBJECT                            40071
#define LW_ERROR_DUPLICATE_USER_OR_GROUP                   40072
#define LW_ERROR_INVALID_KRB5_CACHE_TYPE                   40073
#define LW_ERROR_NOT_JOINED_TO_AD                          40074
#define LW_ERROR_FAILED_TO_SET_TIME                        40075
#define LW_ERROR_NO_NETBIOS_NAME                           40076
#define LW_ERROR_INVALID_NETLOGON_RESPONSE                 40077
#define LW_ERROR_INVALID_OBJECTGUID                        40078
#define LW_ERROR_INVALID_DOMAIN                            40079
#define LW_ERROR_NO_DEFAULT_REALM                          40080
#define LW_ERROR_NOT_SUPPORTED                             40081
#define LW_ERROR_LOGON_FAILURE                             40082
#define LW_ERROR_NO_SITE_INFORMATION                       40083
#define LW_ERROR_INVALID_LDAP_STRING                       40084
#define LW_ERROR_INVALID_LDAP_ATTR_VALUE                   40085
#define LW_ERROR_NULL_BUFFER                               40086
#define LW_ERROR_CLOCK_SKEW                                40087
#define LW_ERROR_KRB5_NO_KEYS_FOUND                        40088
#define LW_ERROR_SERVICE_NOT_AVAILABLE                     40089
#define LW_ERROR_INVALID_SERVICE_RESPONSE                  40090
#define LW_ERROR_NSS_ERROR                                 40091
#define LW_ERROR_AUTH_ERROR                                40092
#define LW_ERROR_INVALID_LDAP_DN                           40093
#define LW_ERROR_NOT_MAPPED                                40094
#define LW_ERROR_RPC_NETLOGON_FAILED                       40095
#define LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED                 40096
#define LW_ERROR_RPC_LSABINDING_FAILED                     40097
#define LW_ERROR_RPC_OPENPOLICY_FAILED                     40098
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED                40099
#define LW_ERROR_RPC_SET_SESS_CREDS_FAILED                 40100
#define LW_ERROR_RPC_REL_SESS_CREDS_FAILED                 40101
#define LW_ERROR_RPC_CLOSEPOLICY_FAILED                    40102
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND             40103
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES      40104
#define LW_ERROR_NO_TRUSTED_DOMAIN_FOUND                   40105
#define LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS 40106
#define LW_ERROR_DCE_CALL_FAILED                           40107
#define LW_ERROR_FAILED_TO_LOOKUP_DC                       40108
#define LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL           40109
#define LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL            40110
#define LW_ERROR_INVALID_USER_NAME                         40111
#define LW_ERROR_INVALID_LOG_LEVEL                         40112
#define LW_ERROR_INVALID_METRIC_TYPE                       40113
#define LW_ERROR_INVALID_METRIC_PACK                       40114
#define LW_ERROR_INVALID_METRIC_INFO_LEVEL                 40115
#define LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK         40116
#define LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED                 40117
#define LW_ERROR_LSA_SERVER_UNREACHABLE                    40118
#define LW_ERROR_INVALID_NSS_ARTEFACT_TYPE                 40119
#define LW_ERROR_INVALID_AGENT_VERSION                     40120
#define LW_ERROR_DOMAIN_IS_OFFLINE                         40121
#define LW_ERROR_INVALID_HOMEDIR_TEMPLATE                  40122
#define LW_ERROR_RPC_PARSE_SID_STRING                      40123
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED                 40124
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND              40125
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES       40126
#define LW_ERROR_PASSWORD_RESTRICTION                      40127
#define LW_ERROR_OBJECT_NOT_ENABLED                        40128
#define LW_ERROR_NO_MORE_NSS_ARTEFACTS                     40129
#define LW_ERROR_INVALID_NSS_MAP_NAME                      40130
#define LW_ERROR_INVALID_NSS_KEY_NAME                      40131
#define LW_ERROR_NO_SUCH_NSS_KEY                           40132
#define LW_ERROR_NO_SUCH_NSS_MAP                           40133
#define LW_ERROR_RPC_ERROR                                 40134
#define LW_ERROR_LDAP_SERVER_UNAVAILABLE                   40135
#define LW_ERROR_CREATE_KEY_FAILED                         40136
#define LW_ERROR_CANNOT_DETECT_USER_PROCESSES              40137
#define LW_ERROR_TRACE_NOT_INITIALIZED                     40138
#define LW_ERROR_NO_SUCH_TRACE_FLAG                        40139
#define LW_ERROR_DCERPC_ERROR                              40140
#define LW_ERROR_INVALID_RPC_SERVER                        40141
#define LW_ERROR_RPC_SERVER_REGISTRATION_ERROR             40142
#define LW_ERROR_RPC_SERVER_RUNTIME_ERROR                  40143
#define LW_ERROR_DOMAIN_IN_USE                             40144
#define LW_ERROR_SAM_DATABASE_ERROR                        40145
#define LW_ERROR_SAM_INIT_ERROR                            40146
#define LW_ERROR_OBJECT_IN_USE                             40147
#define LW_ERROR_NO_SUCH_ATTRIBUTE                         40148
#define LW_ERROR_GET_DC_NAME_FAILED                        40149
#define LW_ERROR_INVALID_ATTRIBUTE_VALUE                   40150
#define LW_ERROR_NO_ATTRIBUTE_VALUE                        40151
#define LW_ERROR_MEMBER_IN_LOCAL_GROUP                     40152
#define LW_ERROR_MEMBER_NOT_IN_LOCAL_GROUP                 40153
#define LW_ERROR_KRB5_S_PRINCIPAL_UNKNOWN                  40154
#define LW_ERROR_INVALID_GROUP                             40155
#define LW_WARNING_CONTINUE_NEEDED                         40157
#define LW_ERROR_ACCESS_DENIED                             40158
#define LW_ERROR_NO_SUCH_PROCESS                           40159
#define LW_ERROR_INTERRUPTED                               40160
#define LW_ERROR_GENERIC_IO                                40161
#define LW_ERROR_INVALID_HANDLE                            40162

#define LW_ERROR_ERRNO_ENXIO                               40163
#define LW_ERROR_ERRNO_E2BIG                               40164
#define LW_ERROR_ERRNO_ENOEXEC                             40165
#define LW_ERROR_ERRNO_ECHILD                              40166
#define LW_ERROR_ERRNO_EAGAIN                              40167
#define LW_ERROR_ERRNO_EFAULT                              40168
#define LW_ERROR_ERRNO_ENOTBLK                             40169
#define LW_ERROR_ERRNO_EBUSY                               40170
#define LW_ERROR_ERRNO_EEXIST                              40171
#define LW_ERROR_ERRNO_EXDEV                               40172
#define LW_ERROR_ERRNO_ENODEV                              40173
#define LW_ERROR_ERRNO_ENOTDIR                             40174
#define LW_ERROR_ERRNO_EISDIR                              40175
#define LW_ERROR_ERRNO_ENFILE                              40176
#define LW_ERROR_ERRNO_EMFILE                              40177
#define LW_ERROR_ERRNO_ENOTTY                              40178
#define LW_ERROR_ERRNO_ETXTBSY                             40179
#define LW_ERROR_ERRNO_EFBIG                               40180
#define LW_ERROR_ERRNO_ENOSPC                              40181
#define LW_ERROR_ERRNO_ESPIPE                              40182
#define LW_ERROR_ERRNO_EROFS                               40183
#define LW_ERROR_ERRNO_EMLINK                              40184
#define LW_ERROR_ERRNO_EPIPE                               40185
#define LW_ERROR_ERRNO_EDOM                                40186
#define LW_ERROR_ERRNO_ERANGE                              40187
#define LW_ERROR_UNKNOWN                                   40188
#define LW_ERROR_ERRNO_ENAMETOOLONG                        40190
#define LW_ERROR_ERRNO_ENOLCK                              40191
#define LW_ERROR_ERRNO_ENOTEMPTY                           40193
#define LW_ERROR_ERRNO_ELOOP                               40194
#define LW_ERROR_ERRNO_ENOMSG                              40196
#define LW_ERROR_ERRNO_EIDRM                               40197
#define LW_ERROR_ERRNO_ECHRNG                              40198
#define LW_ERROR_ERRNO_EL2NSYNC                            40199
#define LW_ERROR_ERRNO_EL3HLT                              40200
#define LW_ERROR_ERRNO_EL3RST                              40201
#define LW_ERROR_ERRNO_ELNRNG                              40202
#define LW_ERROR_ERRNO_EUNATCH                             40203
#define LW_ERROR_ERRNO_ENOCSI                              40204
#define LW_ERROR_ERRNO_EL2HLT                              40205
#define LW_ERROR_ERRNO_EBADE                               40206
#define LW_ERROR_ERRNO_EBADR                               40207
#define LW_ERROR_ERRNO_EXFULL                              40208
#define LW_ERROR_ERRNO_ENOANO                              40209
#define LW_ERROR_ERRNO_EBADRQC                             40210
#define LW_ERROR_ERRNO_EBADSLT                             40211
#define LW_ERROR_ERRNO_EDEADLOCK                           40212
#define LW_ERROR_ERRNO_EBFONT                              40213
#define LW_ERROR_ERRNO_ENOSTR                              40214
#define LW_ERROR_ERRNO_ENODATA                             40215
#define LW_ERROR_ERRNO_ETIME                               40216
#define LW_ERROR_ERRNO_ENOSR                               40217
#define LW_ERROR_ERRNO_ENONET                              40218
#define LW_ERROR_ERRNO_ENOPKG                              40219
#define LW_ERROR_ERRNO_EREMOTE                             40220
#define LW_ERROR_ERRNO_ENOLINK                             40221
#define LW_ERROR_ERRNO_EADV                                40222
#define LW_ERROR_ERRNO_ESRMNT                              40223
#define LW_ERROR_ERRNO_ECOMM                               40224
#define LW_ERROR_ERRNO_EPROTO                              40225
#define LW_ERROR_ERRNO_EMULTIHOP                           40226
#define LW_ERROR_ERRNO_EDOTDOT                             40227
#define LW_ERROR_ERRNO_EBADMSG                             40228
#define LW_ERROR_ERRNO_EOVERFLOW                           40229
#define LW_ERROR_ERRNO_ENOTUNIQ                            40230
#define LW_ERROR_ERRNO_EBADFD                              40231
#define LW_ERROR_ERRNO_EREMCHG                             40232
#define LW_ERROR_ERRNO_ELIBACC                             40233
#define LW_ERROR_ERRNO_ELIBBAD                             40234
#define LW_ERROR_ERRNO_ELIBSCN                             40235
#define LW_ERROR_ERRNO_ELIBMAX                             40236
#define LW_ERROR_ERRNO_ELIBEXEC                            40237
#define LW_ERROR_ERRNO_EILSEQ                              40238
#define LW_ERROR_ERRNO_ERESTART                            40239
#define LW_ERROR_ERRNO_ESTRPIPE                            40240
#define LW_ERROR_ERRNO_EUSERS                              40241
#define LW_ERROR_ERRNO_ENOTSOCK                            40242
#define LW_ERROR_ERRNO_EDESTADDRREQ                        40243
#define LW_ERROR_ERRNO_EMSGSIZE                            40244
#define LW_ERROR_ERRNO_EPROTOTYPE                          40245
#define LW_ERROR_ERRNO_ENOPROTOOPT                         40246
#define LW_ERROR_ERRNO_EPROTONOSUPPORT                     40247
#define LW_ERROR_ERRNO_ESOCKTNOSUPPORT                     40248
#define LW_ERROR_ERRNO_EOPNOTSUPP                          40249
#define LW_ERROR_ERRNO_EPFNOSUPPORT                        40250
#define LW_ERROR_ERRNO_EAFNOSUPPORT                        40251
#define LW_ERROR_ERRNO_EADDRINUSE                          40252
#define LW_ERROR_ERRNO_EADDRNOTAVAIL                       40253
#define LW_ERROR_ERRNO_ENETDOWN                            40254
#define LW_ERROR_ERRNO_ENETUNREACH                         40255
#define LW_ERROR_ERRNO_ENETRESET                           40256
#define LW_ERROR_ERRNO_ECONNABORTED                        40257
#define LW_ERROR_ERRNO_ECONNRESET                          40258
#define LW_ERROR_ERRNO_ENOBUFS                             40259
#define LW_ERROR_ERRNO_EISCONN                             40260
#define LW_ERROR_ERRNO_ENOTCONN                            40261
#define LW_ERROR_ERRNO_ESHUTDOWN                           40262
#define LW_ERROR_ERRNO_ETOOMANYREFS                        40263
#define LW_ERROR_ERRNO_ETIMEDOUT                           40264
#define LW_ERROR_ERRNO_ECONNREFUSED                        40265
#define LW_ERROR_ERRNO_EHOSTDOWN                           40266
#define LW_ERROR_ERRNO_EHOSTUNREACH                        40267
#define LW_ERROR_ERRNO_EALREADY                            40268
#define LW_ERROR_ERRNO_EINPROGRESS                         40269
#define LW_ERROR_ERRNO_ESTALE                              40270
#define LW_ERROR_ERRNO_EUCLEAN                             40271
#define LW_ERROR_ERRNO_ENOTNAM                             40272
#define LW_ERROR_ERRNO_ENAVAIL                             40273
#define LW_ERROR_ERRNO_EISNAM                              40274
#define LW_ERROR_ERRNO_EREMOTEIO                           40275
#define LW_ERROR_ERRNO_EDQUOT                              40276
#define LW_ERROR_ERRNO_ENOMEDIUM                           40277
#define LW_ERROR_ERRNO_EMEDIUMTYPE                         40278
#define LW_ERROR_ERRNO_ECANCELED                           40279

#define LW_ERROR_LDAP_SERVER_DOWN                          40286
#define LW_ERROR_LDAP_LOCAL_ERROR                          40287
#define LW_ERROR_LDAP_ENCODING_ERROR                       40288
#define LW_ERROR_LDAP_DECODING_ERROR                       40289
#define LW_ERROR_LDAP_TIMEOUT                              40290
#define LW_ERROR_LDAP_AUTH_UNKNOWN                         40291
#define LW_ERROR_LDAP_FILTER_ERROR                         40292
#define LW_ERROR_LDAP_USER_CANCELLED                       40293
#define LW_ERROR_LDAP_PARAM_ERROR                          40294
#define LW_ERROR_LDAP_NO_MEMORY                            40295
#define LW_ERROR_LDAP_CONNECT_ERROR                        40296
#define LW_ERROR_LDAP_NOT_SUPPORTED                        40297
#define LW_ERROR_LDAP_CONTROL_NOT_FOUND                    40298
#define LW_ERROR_LDAP_NO_RESULTS_RETURNED                  40299
#define LW_ERROR_LDAP_MORE_RESULTS_TO_RETURN               40300
#define LW_ERROR_LDAP_CLIENT_LOOP                          40301
#define LW_ERROR_LDAP_REFERRAL_LIMIT_EXCEEDED              40302
#define LW_ERROR_LDAP_OPERATIONS_ERROR                     40303
#define LW_ERROR_LDAP_PROTOCOL_ERROR                       40304
#define LW_ERROR_LDAP_TIMELIMIT_EXCEEDED                   40305
#define LW_ERROR_LDAP_SIZELIMIT_EXCEEDED                   40306
#define LW_ERROR_LDAP_COMPARE_FALSE                        40307
#define LW_ERROR_LDAP_COMPARE_TRUE                         40308
#define LW_ERROR_LDAP_STRONG_AUTH_NOT_SUPPORTED            40309
#define LW_ERROR_LDAP_STRONG_AUTH_REQUIRED                 40310
#define LW_ERROR_LDAP_PARTIAL_RESULTS                      40311
#define LW_ERROR_LDAP_NO_SUCH_ATTRIBUTE                    40312
#define LW_ERROR_LDAP_UNDEFINED_TYPE                       40313
#define LW_ERROR_LDAP_INAPPROPRIATE_MATCHING               40314
#define LW_ERROR_LDAP_CONSTRAINT_VIOLATION                 40315
#define LW_ERROR_LDAP_TYPE_OR_VALUE_EXISTS                 40316
#define LW_ERROR_LDAP_INVALID_SYNTAX                       40317


/* range 40500 - 40600 are reserved for GSS specific errors */

#define LW_ERROR_BAD_MECH                                  40500
#define LW_ERROR_BAD_NAMETYPE                              40501
#define LW_ERROR_BAD_NAME                                  40502
#define LW_ERROR_INVALID_CONTEXT                           40503
#define LW_ERROR_INVALID_CREDENTIAL                        40504
#define LW_ERROR_NO_CONTEXT                                40505
#define LW_ERROR_NO_CRED                                   40506
#define LW_ERROR_INVALID_TOKEN                             40507
#define LW_ERROR_UNSUPPORTED_SUBPROTO                      40508
#define LW_ERROR_UNSUPPORTED_CRYPTO_OP                     40509

/* range 40601 - 40699 is reserved for lwtest specific errors */

#define LW_ERROR_TEST_FAILED                               40601
#define LW_ERROR_CSV_BAD_FORMAT                            40602
#define LW_ERROR_CSV_NO_SUCH_FIELD                         40603


/*Range 40700 - 41200 is reserved for registry specific error*/
#define LW_ERROR_DUPLICATE_KEYNAME                         40700
#define LW_ERROR_NO_ACTIVE_KEY_FOUND                       40701
#define LW_ERROR_NO_SUCH_KEY                               40702
#define LW_ERROR_KEY_IS_ACTIVE                             40703
#define LW_ERROR_DUPLICATE_KEYVALUENAME                    40704
#define LW_ERROR_FAILED_DELETE_HAS_SUBKEY                  40705
#define LW_ERROR_NO_SUBKEYS                                40706
#define LW_ERROR_NO_SUCH_VALUENAME                         40707
#define LW_ERROR_UNKNOWN_DATA_TYPE                         40708
#define LW_ERROR_BEYOUND_MAX_VALUE_LEN                     40709
#define LW_ERROR_NO_MORE_ITEMS                             40710


size_t
LwGetErrorString(
    LW_IN LW_DWORD dwError,
    LW_OUT LW_PSTR pszBuffer,
    LW_IN size_t stBufSize
    );

LW_DWORD
LwMapErrnoToLwError(
    LW_IN LW_DWORD dwErrno
    );

LW_DWORD
LwMapHErrnoToLwError(
    LW_IN LW_DWORD dwHErrno
    );

LW_DWORD
LwMapLdapErrorToLwError(
    LW_IN LW_DWORD dwErrno
    );

LW_DWORD
LwMapLwmsgStatusToLwError(
    LW_IN LWMsgStatus status
    );

#endif /* __LW_ERROR_H__ */
