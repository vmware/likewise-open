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
 *        regutils.h
 *
 * Abstract:
 *
 *        Registry system utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __REG_UTILS_H__
#define __REG_UTILS_H__

#ifndef WIN32

/* Special check for parsing error codes */

#define BAIL_ON_REG_PARSE_ERROR(dwError)              \
	if ((dwError != LW_ERROR_SUCCESS) &&             \
	    (dwError != REG_ERROR_INSUFFICIENT_BUFFER)) { \
		REG_LOG_DEBUG("Error at %s:%d [code: %d]",    \
			      __FILE__, __LINE__, dwError);       \
		goto error;                                   \
	}

#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(err)                            \
    do {                                                  \
        if ((err) != STATUS_SUCCESS) {                    \
            REG_LOG_DEBUG("Error at %s:%d [code: %d]",    \
                            __FILE__, __LINE__, dwError); \
		    goto error;                                   \
	    }                                                 \
    } while (0);
#endif

#define BAIL_ON_REG_ERROR(dwError)                                              \
    if (dwError) {                                                              \
       REG_LOG_DEBUG("Error at %s:%d [code: %d]", __FILE__, __LINE__, dwError); \
       goto error;                                                              \
    }

#define BAIL_ON_LWMSG_ERROR(pAssoc, dwError)                                         \
    do {                                                                             \
        if (dwError)                                                                 \
        {                                                                            \
           (dwError) = RegTranslateLwMsgError(pAssoc, dwError, __FILE__, __LINE__); \
           goto error;                                                               \
        }                                                                            \
    } while (0)


#define BAIL_ON_INVALID_STRING(pszParam)          \
        if (IsNullOrEmptyString(pszParam)) {      \
           dwError = LWREG_ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }


#define BAIL_ON_INVALID_HANDLE(hParam)            \
        if (hParam == (HANDLE)NULL) {             \
           dwError = LWREG_ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = LWREG_ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_KEY(pKey)                 \
        BAIL_ON_INVALID_POINTER(pKey);            \
        if (LW_IS_NULL_OR_EMPTY_STR(pKey->pszKeyName)) \
        { \
            dwError = LWREG_ERROR_INVALID_PARAMETER; \
            BAIL_ON_REG_ERROR(dwError); \
        }

#define BAIL_ON_INVALID_REG_ENTRY(pRegEntry)                 \
        BAIL_ON_INVALID_POINTER(pRegEntry);            \
        if (LW_IS_NULL_OR_EMPTY_STR(pRegEntry->pszKeyName)) \
        { \
            dwError = LWREG_ERROR_INVALID_PARAMETER; \
            BAIL_ON_REG_ERROR(dwError); \
        }

#define LWREG_SAFE_FREE_MEMORY(mem) \
    do { \
        if (mem) \
        { \
            LwRtlMemoryFree(mem); \
            (mem) = NULL; \
        } \
    } while (0)

#define LWREG_SAFE_FREE_STRING(str) \
    do { \
        if (str) \
        { \
            RegFreeString(str); \
            (str) = NULL; \
        } \
    } while (0)

#define LW_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))

typedef struct __REG_BIT_VECTOR
{
    DWORD  dwNumBits;
    PDWORD pVector;
} REG_BIT_VECTOR, *PREG_BIT_VECTOR;

typedef struct __REG_HASH_ENTRY REG_HASH_ENTRY;

typedef int (*REG_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*REG_HASH_KEY)(PCVOID);
typedef void (*REG_HASH_FREE_ENTRY)(const REG_HASH_ENTRY *);
typedef DWORD (*REG_HASH_COPY_ENTRY)(const REG_HASH_ENTRY *, REG_HASH_ENTRY *);

struct __REG_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    //Next value in chain
    struct __REG_HASH_ENTRY *pNext;
};

typedef struct __REG_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    REG_HASH_ENTRY **ppEntries;
    REG_HASH_KEY_COMPARE fnComparator;
    REG_HASH_KEY fnHash;
    REG_HASH_FREE_ENTRY fnFree;
    REG_HASH_COPY_ENTRY fnCopy;
} REG_HASH_TABLE, *PREG_HASH_TABLE;

typedef struct __REG_HASH_ITERATOR
{
    REG_HASH_TABLE *pTable;
    size_t sEntryIndex;
    REG_HASH_ENTRY *pEntryPos;
} REG_HASH_ITERATOR;

#define _LW_REG_ASSERT(Expression, Action)                            \
    do {                                                           \
        if (!(Expression))                                         \
        {                                                          \
            REG_LOG_DEBUG("Assertion failed: '" # Expression "'"); \
            Action;                                                \
        }                                                          \
    } while (0)

#define _LW_REG_ASSERT_OR_BAIL(Expression, dwError, Action) \
    _LW_REG_ASSERT(Expression,                              \
                (dwError) = LWREG_ERROR_INTERNAL;          \
                Action ;                                 \
                BAIL_ON_REG_ERROR(dwError))

/*
 * Logging
 */
#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gLogLock;

#define REG_LOCK_LOGGER   pthread_mutex_lock(&gLogLock)
#define REG_UNLOCK_LOGGER pthread_mutex_unlock(&gLogLock)

#if defined (__LWI_DARWIN_X64__)
#define _REG_LOG_WITH_THREAD(Level, Format, ...)   \
    _REG_LOG_MESSAGE(Level,                        \
                     "%zd:" Format,                \
                     (size_t)pthread_self(),       \
                     ## __VA_ARGS__)
#else
#define _REG_LOG_WITH_THREAD(Level, Format, ...)   \
    _REG_LOG_MESSAGE(Level,                        \
                     "0x%x:" Format,               \
                     (unsigned int)pthread_self(), \
                     ## __VA_ARGS__)
#endif

#else

#define REG_LOCK_LOGGER
#define REG_UNLOCK_LOGGER

#define _REG_LOG_WITH_THREAD(Level, Format, ...) \
    _REG_LOG_MESSAGE(Level,                      \
                     Format,                     \
                     ## __VA_ARGS__)

#endif

#define _REG_LOG_WITH_DEBUG(Level, Format, ...)  \
    _REG_LOG_WITH_THREAD(Level,                  \
                         "[%s() %s:%d] " Format, \
                         __FUNCTION__,           \
                         __FILE__,               \
                         __LINE__,               \
                         ## __VA_ARGS__)

extern HANDLE              ghRegLog;
extern RegLogLevel         gRegMaxLogLevel;
extern PFN_REG_LOG_MESSAGE gpfnRegLogger;

#define _REG_LOG_MESSAGE(Level, Format, ...)                        \
    RegLogMessage(gpfnRegLogger, ghRegLog, Level, Format, ## __VA_ARGS__)

#define _REG_LOG_IF(Level, Format, ...)                              \
    do {                                                             \
        REG_LOCK_LOGGER;                                             \
        if (gpfnRegLogger && (gRegMaxLogLevel >= (Level)))              \
        {                                                            \
            if (gRegMaxLogLevel >= REG_LOG_LEVEL_DEBUG)              \
            {                                                        \
                _REG_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__);  \
            }                                                        \
            else                                                     \
            {                                                        \
                _REG_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                                        \
        }                                                            \
        REG_UNLOCK_LOGGER;                                           \
    } while (0)

#define REG_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define REG_LOG_ALWAYS(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define REG_LOG_ERROR(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define REG_LOG_WARNING(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define REG_LOG_INFO(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define REG_LOG_VERBOSE(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define REG_LOG_DEBUG(szFmt, ...)                           \
    _REG_LOG_IF(REG_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gTraceLock;

#define REG_LOCK_TRACER   pthread_mutex_lock(&gTraceLock)
#define REG_UNLOCK_TRACER pthread_mutex_unlock(&gTraceLock)

#else

#define REG_LOCK_TRACER
#define REG_UNLOCK_TRACER

#endif

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#ifdef NDEBUG
#define LW_REG_ASSERT(Expression)
#define LW_REG_ASSERT_OR_BAIL(Expression, dwError) \
    _LW_REG_ASSERT_OR_BAIL(Expression, dwError, 0)
#else
#define LW_REG_ASSERT(Expression) \
    _LW_REG_ASSERT(Expression, abort())
#define LW_REG_ASSERT_OR_BAIL(Expression, dwError)       \
    _LW_REG_ASSERT_OR_BAIL(Expression, dwError, abort())
#endif

#endif //WIN32

typedef struct __DLINKEDLIST
{
    PVOID pItem;

    struct __DLINKEDLIST * pNext;

    struct __DLINKEDLIST * pPrev;

} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_REG_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LWREG_STACK
{
    PVOID pItem;

    struct __LWREG_STACK * pNext;

} LWREG_STACK, *PLWREG_STACK;

//defined flags in dwOptions
#define REG_CFG_OPTION_STRIP_SECTION          0x00000001
#define REG_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define REG_CFG_OPTION_STRIP_ALL (REG_CFG_OPTION_STRIP_SECTION |      \
                                     REG_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

typedef struct _REG_STRING_BUFFER
{
    PSTR pszBuffer;
    // length of the string excluding terminating null
    size_t sLen;
    // capacity of the buffer excluding terminating null
    size_t sCapacity;
} REG_STRING_BUFFER;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNREG_CONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

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
RegRemoveFile(
    PCSTR pszPath
    );

DWORD
RegCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
RegCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbSockExists
    );

DWORD
RegCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbLinkExists
    );

DWORD
RegCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
RegMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RegChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RegChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
RegChangeDirectory(
    PSTR pszPath
    );

DWORD
RegRemoveDirectory(
    PSTR pszPath
    );

DWORD
RegCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
RegGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
RegCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RegGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    );

DWORD
RegGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
RegCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
RegCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RegChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
RegBackupFile(
    PCSTR pszPath
    );

DWORD
RegGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
RegCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
RegCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
RegGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
RegInitLogging(
    PCSTR         pszProgramName,
    RegLogTarget  logTarget,
    RegLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    );

DWORD
RegLogGetInfo(
    PREG_LOG_INFO* ppLogInfo
    );

DWORD
RegLogSetInfo(
    PREG_LOG_INFO pLogInfo
    );

void
reg_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

DWORD
RegShutdownLogging(
    VOID
    );

VOID
RegLogMessage(
    PFN_REG_LOG_MESSAGE pfnLogger,
    HANDLE hLog,
    RegLogLevel logLevel,
    PCSTR  pszFormat,
    ...
    );

DWORD
RegDLinkedList(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
RegDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
RegDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
RegDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
RegDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
RegDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
RegHashCreate(
    size_t sTableSize,
    REG_HASH_KEY_COMPARE fnComparator,
    REG_HASH_KEY fnHash,
    REG_HASH_FREE_ENTRY fnFree, //optional
    REG_HASH_COPY_ENTRY fnCopy, //optional
    REG_HASH_TABLE** ppResult
    );

void
RegHashRemoveAll(
    REG_HASH_TABLE* pResult
    );

void
RegHashSafeFree(
    REG_HASH_TABLE** ppResult
    );

DWORD
RegHashSetValue(
    REG_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ENOENT if pKey is not in the table
DWORD
RegHashGetValue(
    REG_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
RegHashExists(
    IN PREG_HASH_TABLE pTable,
    IN PCVOID pKey
    );

DWORD
RegHashCopy(
    IN  REG_HASH_TABLE *pTable,
    OUT REG_HASH_TABLE **ppResult
    );

//Invalidates all iterators
DWORD
RegHashResize(
    REG_HASH_TABLE *pTable,
    size_t sTableSize
    );

VOID
RegHashGetIterator(
    REG_HASH_TABLE *pTable,
    REG_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
REG_HASH_ENTRY *
RegHashNext(
    REG_HASH_ITERATOR *pIterator
    );

DWORD
RegHashRemoveKey(
    REG_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
RegHashCaselessStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
RegHashCaselessStringHash(
    PCVOID str
    );

int
RegHashPVoidCompare(
    IN PCVOID pvData1,
    IN PCVOID pvData2
    );

size_t
RegHashPVoidHash(
    IN PCVOID pvData
    );

VOID
RegHashFreeStringKey(
    IN OUT const REG_HASH_ENTRY *pEntry
    );

DWORD
RegInitializeStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    size_t sCapacity
    );

DWORD
RegAppendStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    PCSTR pszAppend
    );

void
RegFreeStringBufferContents(
    REG_STRING_BUFFER *pBuffer
    );

VOID
RegPrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    );

DWORD
RegMapErrnoToLwRegError(
    DWORD dwErrno
    );

DWORD
RegNtStatusToWin32Error(
    NTSTATUS ntStatus
    );

DWORD
RegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    );

void
RegFreeString(
    PSTR pszString
    );

void
RegFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

DWORD
RegStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

DWORD
RegHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    );

DWORD
RegByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
RegStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

void
RegStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

#endif /* __REG_UTILS_H__ */
