#include "includes.h"

PSTR gpszRdrProviderName = "re-director";

NTVFS_DRIVER gRdrProviderTable =
{
        &RdrCreateFileEx,
        &RdrReadFileEx,
        &RdrWriteFileEx,
        &RdrGetSessionKey,
        &RdrCloseFileEx,
        &RdrTreeConnect,
        &RdrNTCreate,
        &RdrNTTransactCreate,
        &RdrCreateTemporary,
        &RdrReadFile,
        &RdrWriteFile,
        &RdrLockFile,
        &RdrSeekFile,
        &RdrFlushFile,
        &RdrCloseFile,
        &RdrCloseFileAndDisconnect,
        &RdrDeleteFile,
        &RdrRenameFile,
        &RdrCopyFile,
        &RdrTrans2QueryFileInformation,
        &RdrTrans2SetPathInformation,
        &RdrTrans2QueryPathInformation,
        &RdrTrans2CreateDirectory,
        &RdrTrans2DeleteDirectory,
        &RdrTrans2CheckDirectory,
        &RdrTrans2FindFirst2,
        &RdrTrans2FindNext2,
        &RdrNTTransactNotifyChange,
        &RdrTrans2GetDFSReferral
};

/* Socket hash by name */
PSMB_HASH_TABLE   gpSocketHashByName = NULL;

/* Socket hash by address */
PSMB_HASH_TABLE   gpSocketHashByAddress = NULL;

/* Protects both hashes */
pthread_rwlock_t  gSocketHashLock;

/* Stack of reapers */
PSMB_STACK  gpReaperStack = NULL;

BOOLEAN     gSignMessagesIfSupported = TRUE;
