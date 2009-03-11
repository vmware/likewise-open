#ifndef __SAMDB_CONTEXT_H__
#define __SAMDB_CONTEXT_H__

DWORD
SamDbBuildDirectoryContext(
    PSAM_DB_INSTANCE_LOCK pDbInstanceLock,
    PSAM_DIRECTORY_CONTEXT* ppDirContext
    );

VOID
SamDbFreeDirectoryContext(
    PSAM_DIRECTORY_CONTEXT pDirContext
    );

#endif /* __SAMDB_CONTEXT_H__ */
