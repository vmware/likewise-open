#ifndef __HANDLEMGR_P_H__
#define __HANDLEMGR_P_H__

typedef struct __SMB_HANDLE_TABLE_ENTRY
{
    SMBHandleType hType;
    PVOID         pItem;
} SMB_HANDLE_TABLE_ENTRY, *PSMB_HANDLE_TABLE_ENTRY;

#endif
