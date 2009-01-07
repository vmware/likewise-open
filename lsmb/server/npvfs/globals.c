#include "includes.h"

PSTR  gpszNPVFSProviderName = "Named Pipes Virtual File System";
NTVFS_DRIVER gNPVFSProviderTable =
{
        &NPVfsCreateFileEx,
        &NPVfsReadFileEx,
        &NPVfsWriteFileEx,
        &NPVfsGetSessionKey,
        &NPVfsCloseFileEx,
        &NPVfsTreeConnect,
        &NPVfsNTCreate,
        &NPVfsNTTransactCreate,
        &NPVfsCreateTemporary,
        &NPVfsReadFile,
        &NPVfsWriteFile,
        &NPVfsLockFile,
        &NPVfsSeekFile,
        &NPVfsFlushFile,
        &NPVfsCloseFile,
        &NPVfsCloseFileAndDisconnect,
        &NPVfsDeleteFile,
        &NPVfsRenameFile,
        &NPVfsCopyFile,
        &NPVfsTrans2QueryFileInformation,
        &NPVfsTrans2SetPathInformation,
        &NPVfsTrans2QueryPathInformation,
        &NPVfsTrans2CreateDirectory,
        &NPVfsTrans2DeleteDirectory,
        &NPVfsTrans2CheckDirectory,
        &NPVfsTrans2FindFirst2,
        &NPVfsTrans2FindNext2,
        &NPVfsNTTransactNotifyChange,
        &NPVfsTrans2GetDFSReferral
};
