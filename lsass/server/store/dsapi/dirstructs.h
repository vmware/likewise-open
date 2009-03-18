#ifndef __DIRSTRUCTS_H__
#define __DIRSTRUCTS_H__

typedef struct _DIRECTORY_CONTEXT
{
    DirectoryType directoryType;
    HANDLE        hBindHandle;
} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

#endif /* __DIRSTRUCTS_H__ */
