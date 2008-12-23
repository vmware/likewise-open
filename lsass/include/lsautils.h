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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsautils.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) system utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LSA_UTILS_H__
#define __LSA_UTILS_H__

#include "lsaipc.h"
#include "lsalist.h"

#ifndef LW_ENDIAN_SWAP16

#define LW_ENDIAN_SWAP16(wX)                     \
        ((((UINT16)(wX) & 0xFF00) >> 8) |        \
         (((UINT16)(wX) & 0x00FF) << 8))

#endif

#ifndef LW_ENDIAN_SWAP32

#define LW_ENDIAN_SWAP32(dwX)                    \
        ((((UINT32)(dwX) & 0xFF000000L) >> 24) | \
         (((UINT32)(dwX) & 0x00FF0000L) >>  8) | \
         (((UINT32)(dwX) & 0x0000FF00L) <<  8) | \
         (((UINT32)(dwX) & 0x000000FFL) << 24))

#endif

#ifndef LW_ENDIAN_SWAP64

#define LW_ENDIAN_SWAP64(llX)         \
   (((UINT64)(LW_ENDIAN_SWAP32(((UINT64)(llX) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((UINT64)LW_ENDIAN_SWAP32(((UINT64)(llX) & 0x00000000FFFFFFFFLL))) << 32))

#endif

#ifndef WIN32

/* Special check for parsing error codes */

#define BAIL_ON_LSA_PARSE_ERROR(dwError) \
	if ((dwError != LSA_ERROR_SUCCESS) &&				\
	    (dwError != LSA_ERROR_INSUFFICIENT_BUFFER)) {		\
		LSA_LOG_DEBUG("Error at %s:%d [code: %d]",		\
			      __FILE__, __LINE__, dwError);		\
		goto error;						\
	}

#define BAIL_ON_LSA_ERROR(dwError) \
    if (dwError) {\
       LSA_LOG_DEBUG("Error at %s:%d [code: %d]", __FILE__, __LINE__, dwError); \
       goto error;                 \
    }

#define BAIL_ON_LWMSG_ERROR(pAssoc, dwError) \
    do { \
        if (dwError) \
        { \
           (dwError) = LsaTranslateLwMsgError(pAssoc, dwError, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#define BAIL_WITH_LSA_ERROR(_newerror_) \
    do {dwError = (_newerror_); BAIL_ON_LSA_ERROR(dwError);} while (0)

#endif

#define LSA_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              LsaFreeString(str); \
              (str) = NULL;       \
           }                      \
        } while(0);

#define LSA_SAFE_CLEAR_FREE_STRING(str)       \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              LsaFreeString(str);             \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define LSA_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              LsaFreeMemory(mem); \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define LSA_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                \
           if (ppszArray) {                                 \
               LsaFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                          \
           }                                                \
        } while (0);

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

/*
 * Logging
 */
#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gLogLock;

#define LSA_LOCK_LOGGER   pthread_mutex_lock(&gLogLock)
#define LSA_UNLOCK_LOGGER pthread_mutex_unlock(&gLogLock)

#define _LSA_LOG_WITH_THREAD(Level, Format, ...) \
    _LSA_LOG_MESSAGE(Level, \
                     "0x%x:" Format, \
                     (unsigned int)pthread_self(), \
                     ## __VA_ARGS__)

#else

#define LSA_LOCK_LOGGER
#define LSA_UNLOCK_LOGGER

#define _LSA_LOG_WITH_THREAD(Level, Format, ...) \
    _LSA_LOG_MESSAGE(Level, \
                     Format, \
                     ## __VA_ARGS__)

#endif

#define _LSA_LOG_WITH_DEBUG(Level, Format, ...) \
    _LSA_LOG_WITH_THREAD(Level, \
                         "[%s() %s:%d] " Format, \
                         __FUNCTION__, \
                         __FILE__, \
                         __LINE__, \
                         ## __VA_ARGS__)

extern HANDLE              ghLog;
extern LsaLogLevel         gLsaMaxLogLevel;
extern PFN_LSA_LOG_MESSAGE gpfnLogger;

#define _LSA_LOG_MESSAGE(Level, Format, ...) \
    LsaLogMessage(gpfnLogger, ghLog, Level, Format, ## __VA_ARGS__)

#define _LSA_LOG_IF(Level, Format, ...)                     \
    do {                                                    \
        LSA_LOCK_LOGGER;                                    \
        if (gpfnLogger && (gLsaMaxLogLevel >= (Level)))     \
        {                                                   \
            if (gLsaMaxLogLevel >= LSA_LOG_LEVEL_DEBUG)     \
            {                                               \
                _LSA_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__); \
            }                                               \
            else                                            \
            {                                               \
                _LSA_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                               \
        }                                                   \
        LSA_UNLOCK_LOGGER;                                  \
    } while (0)

#define LSA_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define LSA_LOG_ALWAYS(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define LSA_LOG_ERROR(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define LSA_LOG_WARNING(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define LSA_LOG_INFO(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define LSA_LOG_VERBOSE(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define LSA_LOG_DEBUG(szFmt, ...) \
    _LSA_LOG_IF(LSA_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

// Like assert() but also calls LSA_LOG.
#define _LSA_ASSERT(Expression, Action) \
    do { \
        if (!(Expression)) \
        { \
            LSA_LOG_DEBUG("Assertion failed: '" # Expression "'"); \
            Action; \
        } \
    } while (0)
#define _LSA_ASSERT_OR_BAIL(Expression, dwError, Action) \
    _LSA_ASSERT(Expression, \
                (dwError) = LSA_ERROR_INTERNAL; \
                Action ; \
                BAIL_ON_LSA_ERROR(dwError))
#ifdef NDEBUG
#define LSA_ASSERT(Expression)
#define LSA_ASSERT_OR_BAIL(Expression, dwError) \
    _LSA_ASSERT_OR_BAIL(Expression, dwError, 0)
#else
#define LSA_ASSERT(Expression) \
    _LSA_ASSERT(Expression, abort())
#define LSA_ASSERT_OR_BAIL(Expression, dwError) \
    _LSA_ASSERT_OR_BAIL(Expression, dwError, abort())
#endif

#define LSA_IS_XOR(Expression1, Expression2) \
    (!!(Expression1) ^ !!(Expression2))

#define LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo) \
    do { \
        if (pLoginNameInfo) \
        { \
            LsaFreeNameInfo(pLoginNameInfo); \
            (pLoginNameInfo) = NULL; \
        } \
    } while(0);

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gTraceLock;

#define LSA_LOCK_TRACER   pthread_mutex_lock(&gTraceLock)
#define LSA_UNLOCK_TRACER pthread_mutex_unlock(&gTraceLock)

#else

#define LSA_LOCK_TRACER
#define LSA_UNLOCK_TRACER

#endif

#define LSA_TRACE_BEGIN_FUNCTION(traceFlagArray, dwNumFlags)  \
    do {                                                      \
        LSA_LOCK_TRACER;                                      \
        if (LsaTraceIsAllowed(traceFlagArray, dwNumFlags)) {  \
            LSA_LOG_ALWAYS("Begin %s() %s:%d",                \
                           __FUNCTION__,                      \
                           __FILE__,                          \
                           __LINE__);                         \
        }                                                     \
        LSA_UNLOCK_TRACER;                                    \
    } while (0)

#define LSA_TRACE_END_FUNCTION(traceFlagArray, dwNumFlags)   \
    do {                                                     \
        LSA_LOCK_TRACER;                                     \
        if (LsaTraceIsAllowed(traceFlagArray, dwNumFlags)) { \
            LSA_LOG_ALWAYS("End %s() %s:%d",                 \
                           __FUNCTION__,                     \
                           __FILE__,                         \
                           __LINE__);                        \
        }                                                    \
        LSA_UNLOCK_TRACER;                                   \
    } while (0)

#define SERVICE_LDAP        1
#define SERVICE_KERBEROS    2

typedef struct _DNS_FQDN
{
    PSTR pszFQDN;
    PSTR pszAddress;
} DNS_FQDN, *PDNS_FQDN;

//defined flags in dwOptions
#define LSA_CFG_OPTION_STRIP_SECTION    0x00000001
#define LSA_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define LSA_CFG_OPTION_STRIP_ALL              (LSA_CFG_OPTION_STRIP_SECTION | \
                                              LSA_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

#define INIT_SEC_BUFFER_S(_s_, _l_) do{(_s_)->length = (_s_)->maxLength = (_l_); memset((_s_)->buffer, 0, S_BUFLEN);} while (0)
#define INIT_SEC_BUFFER_S_VAL(_s_, _l_, _v_) do{(_s_)->length = (_s_)->maxLength = (_l_); memcpy((_s_)->buffer,(_v_), _l_);} while (0)
#define SEC_BUFFER_COPY(_d_,_s_) memcpy((_d_)->buffer,(_s_)->buffer,(_s_)->maxLength)
#define SEC_BUFFER_S_CONVERT(_sb_,_sbs_) do{(_sb_)->length = (_sbs_)->length;(_sb_)->maxLength=(_sbs_)->maxLength;(_sb_)->buffer = (_sbs_)->buffer;} while (0)

#define BAIL_ON_INVALID_STRING(pszParam)          \
        if (IsNullOrEmptyString(pszParam)) {      \
           dwError = LSA_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)            \
        if (hParam == (HANDLE)NULL) {             \
           dwError = LSA_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = LSA_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

//format of string representation of SID in SECURITYIDENTIFIER:
//S-<revision>-<authority>-<domain_computer_id>-<relative ID>
//example: S-1-5-32-546 (Guests)
//See http://support.microsoft.com/kb/243330

#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LSA_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;  //byte representation of multi-byte Security Identification Descriptor
    DWORD dwByteLength;
} LSA_SECURITY_IDENTIFIER, *PLSA_SECURITY_IDENTIFIER;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNCONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef struct __LSA_BIT_VECTOR
{
    DWORD  dwNumBits;
    PDWORD pVector;
} LSA_BIT_VECTOR, *PLSA_BIT_VECTOR;

typedef struct __LSA_HASH_ENTRY LSA_HASH_ENTRY;

typedef int (*LSA_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*LSA_HASH_KEY)(PCVOID);
typedef void (*LSA_HASH_FREE_ENTRY)(const LSA_HASH_ENTRY *);

struct __LSA_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    //Next value in chain
    struct __LSA_HASH_ENTRY *pNext;
};

typedef struct __LSA_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    LSA_HASH_ENTRY **ppEntries;
    LSA_HASH_KEY_COMPARE fnComparator;
    LSA_HASH_KEY fnHash;
    LSA_HASH_FREE_ENTRY fnFree;
} LSA_HASH_TABLE, *PLSA_HASH_TABLE;

typedef struct __LSA_HASH_ITERATOR
{
    LSA_HASH_TABLE *pTable;
    size_t sEntryIndex;
    LSA_HASH_ENTRY *pEntryPos;
} LSA_HASH_ITERATOR;

typedef struct __DLINKEDLIST
{
    PVOID pItem;

    struct __DLINKEDLIST * pNext;

    struct __DLINKEDLIST * pPrev;

} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_LSA_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LSA_STACK
{
    PVOID pItem;

    struct __LSA_STACK * pNext;

} LSA_STACK, *PLSA_STACK;

/* wire definition */
typedef struct _SEC_BUFFER_RELATIVE {
    USHORT length;
    USHORT maxLength;
    ULONG  offset;
} SEC_BUFFER_RELATIVE, *PSEC_BUFFER_RELATIVE;

typedef struct _OID {
    DWORD length;
    PVOID *elements;
} OID, *POID;

typedef struct _OID_SET {
    DWORD count;
    POID  elements;
} OID_SET, *POID_SET;

typedef struct _LSA_STRING_BUFFER
{
    PSTR pszBuffer;
    // length of the string excluding terminating null
    size_t sLen;
    // capacity of the buffer excluding terminating null
    size_t sCapacity;
} LSA_STRING_BUFFER;

typedef struct passwd * passwd_ptr_t;
typedef struct group  * group_ptr_t;

typedef enum
{
    NameType_NT4 = 0,
    NameType_UPN = 1,
    NameType_Alias
} ADLogInNameType;

typedef struct __LSA_LOGIN_NAME_INFO
{
    ADLogInNameType nameType;
    PSTR  pszDomainNetBiosName;
    PSTR  pszFullDomainName;
    PSTR  pszName;
    PSTR  pszObjectSid;
} LSA_LOGIN_NAME_INFO, *PLSA_LOGIN_NAME_INFO;

typedef enum
{
    LsaMetricSuccessfulAuthentications    =  0,
    LsaMetricFailedAuthentications        =  1,
    LsaMetricRootUserAuthentications      =  2,
    LsaMetricSuccessfulUserLookupsByName  =  3,
    LsaMetricFailedUserLookupsByName      =  4,
    LsaMetricSuccessfulUserLookupsById    =  5,
    LsaMetricFailedUserLookupsById        =  6,
    LsaMetricSuccessfulGroupLookupsByName =  7,
    LsaMetricFailedGroupLookupsByName     =  8,
    LsaMetricSuccessfulGroupLookupsById   =  9,
    LsaMetricFailedGroupLookupsById       = 10,
    LsaMetricSuccessfulOpenSession        = 11,
    LsaMetricFailedOpenSession            = 12,
    LsaMetricSuccessfulCloseSession       = 13,
    LsaMetricFailedCloseSession           = 14,
    LsaMetricSuccessfulChangePassword     = 15,
    LsaMetricFailedChangePassword         = 16,
    LsaMetricUnauthorizedAccesses         = 17,
    LsaMetricSentinel
} LsaMetricType;

typedef struct __LSA_NIS_NICKNAME
{
    PSTR pszMapAlias;
    PSTR pszMapName;
} LSA_NIS_NICKNAME, *PLSA_NIS_NICKNAME;

typedef DWORD (*PFLSA_CACHE_HASH) (PVOID pKey, DWORD dwIndex, PVOID pData);
typedef BOOLEAN (*PFLSA_CACHE_EQUAL) (PVOID pKey1, PVOID pKey2, DWORD dwIndex, PVOID pData);
typedef PVOID (*PFLSA_CACHE_GETKEY) (PVOID pEntry, DWORD dwIndex, PVOID pData);
typedef DWORD (*PFLSA_CACHE_MISS) (PVOID pKey, DWORD dwIndex, PVOID pData, PVOID* ppEntry);
typedef DWORD (*PFLSA_CACHE_KICK) (PVOID pEntry, PVOID pData);

typedef struct __LSA_CACHE_ENTRY
{
    DWORD dwRefCount;
} LSA_CACHE_ENTRY, *PLSA_CACHE_ENTRY;

typedef struct __LSA_CACHE
{
    DWORD dwNumKeys;
    DWORD dwNumBuckets;
    PVOID* ppEntries;
    PFLSA_CACHE_HASH pfHash;
    PFLSA_CACHE_EQUAL pfEqual;
    PFLSA_CACHE_GETKEY pfGetKey;
    PFLSA_CACHE_MISS pfMiss;
    PFLSA_CACHE_KICK pfKick;
    PVOID pData;
    DWORD dwNumHashMisses;
    DWORD dwNumFullMisses;
    DWORD dwNumKicks;
    DWORD dwNumUsedBuckets;
    DWORD dwNumCollisions;
} LSA_CACHE, *PLSA_CACHE;

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOLL) */

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOULL) */

DWORD
LsaInitializeStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        size_t sCapacity);

DWORD
LsaAppendStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        PCSTR pszAppend);

void
LsaFreeStringBufferContents(
        LSA_STRING_BUFFER *pBuffer);

DWORD
LsaAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
LsaReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );

void
LsaFreeMemory(
    PVOID pMemory
    );

DWORD
LsaAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

void
LsaFreeString(
    PSTR pszString
    );

void
LsaStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

DWORD
LsaStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    );

void
LsaStrToUpper(
    PSTR pszString
    );

void
LsaStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    );

void
LsaStrToLower(
    PSTR pszString
    );

void
LsaStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    );

DWORD
LsaEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
LsaStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

DWORD
LsaAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
LsaAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

VOID
LsaStrCharReplace(
    PSTR pszStr,
    CHAR oldCh,
    CHAR newCh);

// If pszInputString == NULL, then *ppszOutputString = NULL
DWORD
LsaStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );


// If pszInputString == NULL, return "", else return pszInputString
// The return value does not need to be freed.
PCSTR
LsaEmptyStrForNull(
    PCSTR pszInputString
    );


void
LsaStrChr(
    PCSTR pszInputString,
    CHAR c,
    PSTR *pszOutputString
    );

void
LsaFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
LsaFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

VOID
LsaPrincipalNonRealmToLower(
    IN OUT PSTR pszPrincipal
    );

VOID
LsaPrincipalRealmToUpper(
    IN OUT PSTR pszPrincipal
    );

DWORD
LsaBitVectorCreate(
    DWORD dwNumBits,
    PLSA_BIT_VECTOR* ppBitVector
    );

VOID
LsaBitVectorFree(
    PLSA_BIT_VECTOR pBitVector
    );

BOOLEAN
LsaBitVectorIsSet(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LsaBitVectorSetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LsaBitVectorUnsetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

VOID
LsaBitVectorReset(
    PLSA_BIT_VECTOR pBitVector
    );

DWORD
LsaDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
LsaDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
LsaDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
LsaDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
LsaDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
LsaStackPush(
    PVOID pItem,
    PLSA_STACK* ppStack
    );

PVOID
LsaStackPop(
    PLSA_STACK* ppStack
    );

PVOID
LsaStackPeek(
    PLSA_STACK pStack
    );

DWORD
LsaStackForeach(
    PLSA_STACK pStack,
    PFN_LSA_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PLSA_STACK
LsaStackReverse(
    PLSA_STACK pStack
    );

VOID
LsaStackFree(
    PLSA_STACK pStack
    );

DWORD
LsaHashCreate(
    size_t sTableSize,
    LSA_HASH_KEY_COMPARE fnComparator,
    LSA_HASH_KEY fnHash,
    LSA_HASH_FREE_ENTRY fnFree, //optional
    LSA_HASH_TABLE** ppResult
    );

void
LsaHashSafeFree(
    LSA_HASH_TABLE** ppResult
    );

DWORD
LsaHashSetValue(
    LSA_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ENOENT if pKey is not in the table
DWORD
LsaHashGetValue(
    LSA_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
LsaHashExists(
    IN PLSA_HASH_TABLE pTable,
    IN PCVOID pKey
    );

//Invalidates all iterators
DWORD
LsaHashResize(
    LSA_HASH_TABLE *pTable,
    size_t sTableSize
    );

DWORD
LsaHashGetIterator(
    LSA_HASH_TABLE *pTable,
    LSA_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
LSA_HASH_ENTRY *
LsaHashNext(
    LSA_HASH_ITERATOR *pIterator
    );

DWORD
LsaHashRemoveKey(
    LSA_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
LsaHashCaselessStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
LsaHashCaselessString(
    PCVOID str
    );

DWORD
LsaRemoveFile(
    PCSTR pszPath
    );

DWORD
LsaCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LsaCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LsaMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LsaChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LsaChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LsaChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LsaGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LsaChangeDirectory(
    PSTR pszPath
    );

DWORD
LsaRemoveDirectory(
    PSTR pszPath
    );

DWORD
LsaCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
LsaCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
LsaCreateDirectory(
    PSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LsaGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    );

DWORD
LsaCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
LsaGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
LsaCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LsaBackupFile(
    PCSTR pszPath
    );

DWORD
LsaGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
LsaCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
LsaGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
LsaGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
LsaGetLibDirPath(
    PSTR* ppszPath
    );

DWORD
LsaCoalesceGroupInfoList(
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList_accumulate,
    PDWORD  pdwTotalNumGroupsFound
    );

DWORD
LsaValidateGroupInfoLevel(
    DWORD dwGroupInfoLevel
    );

DWORD
LsaValidateGroupName(
    PCSTR pszGroupName
    );

DWORD
LsaValidateGroupInfo(
    PVOID pGroupInfo,
    DWORD dwGroupInfoLevel
    );

DWORD
LsaCoalesceNSSArtefactInfoList(
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList_accumulate,
    PDWORD  pdwTotalNumNSSArtefactsFound
    );

DWORD
LsaValidateUserInfo(
    PVOID pUserInfo,
    DWORD dwUserInfoLevel
    );

DWORD
LsaValidateUserInfoLevel(
    DWORD dwUserInfoLevel
    );

DWORD
LsaCoalesceUserInfoList(
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList_accumulate,
    PDWORD  pdwTotalNumUsersFound
    );

DWORD
LsaValidateUserName(
    PCSTR pszName
    );

#define LSA_DOMAIN_SEPARATOR_DEFAULT    '\\'

CHAR
LsaGetDomainSeparator(
    VOID
    );

DWORD
LsaSetDomainSeparator(
    CHAR chValue
    );

DWORD
LsaCrackDomainQualifiedName(
    PCSTR pszId,
    PCSTR pszDefaultDomain,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    );

void
LsaFreeNameInfo(
    PLSA_LOGIN_NAME_INFO pNameInfo
    );

DWORD
LsaBuildUserModInfo(
    uid_t uid,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

DWORD
LsaModifyUser_EnableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_DisableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_Unlock(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_AddToGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LsaModifyUser_RemoveFromGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LsaModifyUser_ChangePasswordAtNextLogon(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetPasswordNeverExpires(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetPasswordMustExpire(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetAccountExpiryDate(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszDate
    );

void
LsaFreeUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    );

DWORD
LsaDnsGetHostInfo(
    PSTR* ppszHostname
    );

VOID
LsaDnsFreeFQDNList(
    PDNS_FQDN* ppFQDNList,
    DWORD dwNFQDN
    );

VOID
LsaDnsFreeFQDN(
    PDNS_FQDN pFQDN
    );

DWORD
LsaInitLogging(
    PCSTR         pszProgramName,
    LsaLogTarget  logTarget,
    LsaLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    );

DWORD
LsaLogGetInfo(
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LsaLogSetInfo(
    PLSA_LOG_INFO pLogInfo
    );

VOID
LsaLogMessage(
    PFN_LSA_LOG_MESSAGE pfnLogger,
    HANDLE hLog,
    LsaLogLevel logLevel,
    PCSTR  pszFormat,
    ...
    );

void
lsass_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

DWORD
LsaShutdownLogging(
    VOID
    );

DWORD
LsaValidateLogLevel(
    DWORD dwLogLevel
    );

DWORD
LsaTraceInitialize(
    VOID
    );

BOOLEAN
LsaTraceIsFlagSet(
    DWORD dwTraceFlag
    );

BOOLEAN
LsaTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    );

DWORD
LsaTraceSetFlag(
    DWORD dwTraceFlag
    );

DWORD
LsaTraceUnsetFlag(
    DWORD dwTraceFlag
    );

VOID
LsaTraceShutdown(
    VOID
    );

VOID
LsaTraceShutdown(
    VOID
    );

DWORD
LsaAllocSecurityIdentifierFromBinary(
    UCHAR* sidBytes,
    DWORD dwSidBytesLength,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LsaAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LsaFreeSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LsaGetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LsaSetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

DWORD
LsaReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    );

DWORD
LsaGetSecurityIdentifierHashedRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

// The UID is a DWORD constructued using a non-cryptographic hash
// of the User's domain SID and user RID.
DWORD
LsaHashSecurityIdentifierToId(
    IN PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    );

DWORD
LsaHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    );

DWORD
LsaGetSecurityIdentifierBinary(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LsaGetSecurityIdentifierString(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LsaSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PSTR* pszSidString
    );

DWORD
LsaGetDomainSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLSA_SECURITY_IDENTIFIER* ppDomainSID
    );

DWORD
LsaHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD* pdwByteArrayLength
    );

DWORD
LsaByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LsaByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LsaSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    );

int
LsaStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    );

VOID
LsaFreeDCInfo(
    PLSA_DC_INFO pDCInfo
    );

VOID
LsaFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    );

VOID
LsaFreeDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

VOID
LsaFreeDomainInfoContents(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

DWORD
LsaNISGetNicknames(
    PCSTR         pszNicknameFilePath,
    PDLINKEDLIST* ppNicknameList
    );

PCSTR
LsaNISLookupAlias(
    PDLINKEDLIST pNicknameList,
    PCSTR pszAlias
    );

VOID
LsaNISFreeNicknameList(
    PDLINKEDLIST pNicknameList
    );

DWORD
LsaCacheNew(
    DWORD dwNumKeys,
    DWORD dwNumBuckets,
    PFLSA_CACHE_HASH pfHash,
    PFLSA_CACHE_EQUAL pfEqual,
    PFLSA_CACHE_GETKEY pfGetKey,
    PFLSA_CACHE_MISS pfMiss,
    PFLSA_CACHE_KICK pfKick,
    PVOID pData,
    PLSA_CACHE* ppCache
    );

DWORD
LsaCacheInsert(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LsaCacheRemove(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LsaCacheLookup(
    PLSA_CACHE pCache,
    PVOID pkey,
    DWORD dwIndex,
    PVOID* ppEntry
    );

DWORD
LsaCacheFlush(
    PLSA_CACHE pCache
    );

VOID
LsaCacheDelete(
    PLSA_CACHE pCache
    );

DWORD
LsaTranslateLwMsgError(
        LWMsgAssoc *pAssoc,
        DWORD dwMsgError,
        const char *file,
        int line
        );

#endif /* __LSA_UTILS_H__ */


