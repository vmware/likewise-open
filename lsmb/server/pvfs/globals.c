#include "includes.h"

PSTR gpszPVFSProviderName = "Posix Virtual File System";
NTVFS_DRIVER gPVFSProviderTable =
{
        &PVfsCreateFileEx,
        &PVfsReadFileEx,
        &PVfsWriteFileEx,
        &PVfsGetSessionKey,
        &PVfsCloseFileEx,
        &PVfsTreeConnect,
        &PVfsNTCreate,
        &PVfsNTTransactCreate,
        &PVfsCreateTemporary,
        &PVfsReadFile,
        &PVfsWriteFile,
        &PVfsLockFile,
        &PVfsSeekFile,
        &PVfsFlushFile,
        &PVfsCloseFile,
        &PVfsCloseFileAndDisconnect,
        &PVfsDeleteFile,
        &PVfsRenameFile,
        &PVfsCopyFile,
        &PVfsTrans2QueryFileInformation,
        &PVfsTrans2SetPathInformation,
        &PVfsTrans2QueryPathInformation,
        &PVfsTrans2CreateDirectory,
        &PVfsTrans2DeleteDirectory,
        &PVfsTrans2CheckDirectory,
        &PVfsTrans2FindFirst2,
        &PVfsTrans2FindNext2,
        &PVfsNTTransactNotifyChange,
        &PVfsTrans2GetDFSReferral
};
