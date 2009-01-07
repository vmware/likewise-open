
NTSTATUS
SmbProcessNTRename(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
UnmarshallNTRenameRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    );

NTSTATUS
MarshallNTRenameResponse(
    PSMB_CONNECTION pSmbRequest
    );

