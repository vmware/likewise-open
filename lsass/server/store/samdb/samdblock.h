#ifndef __SAMDB_LOCK_H__
#define __SAMDB_LOCK_H__

DWORD
SamDbBuildDbInstanceLock(
    PSAM_DB_INSTANCE_LOCK* ppLock
    );

DWORD
SamDbAcquireDbInstanceLock(
    PSAM_DB_INSTANCE_LOCK pLock,
    PSAM_DB_INSTANCE_LOCK* ppLock
    );

VOID
SamDbReleaseDbInstanceLock(
    PSAM_DB_INSTANCE_LOCK pLock
    );

VOID
SamDbFreeDbInstanceLock(
    PSAM_DB_INSTANCE_LOCK pLock
    );

#endif /* __SAMDB_LOCK_H__ */
