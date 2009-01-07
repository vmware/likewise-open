#ifndef __EXTERNS_H__
#define __EXTERNS_H__

extern PSTR           gpszRdrProviderName;
extern NTVFS_DRIVER   gRdrProviderTable;

extern SMB_HASH_TABLE   *gpSocketHashByName;    /* Socket hash by name */
extern SMB_HASH_TABLE   *gpSocketHashByAddress; /* Socket hash by address */
extern pthread_rwlock_t  gSocketHashLock;       /* Protects both hashes */

extern PSMB_STACK        gpReaperStack;         /* Stack of reapers */

extern BOOLEAN           gSignMessagesIfSupported;

#endif

