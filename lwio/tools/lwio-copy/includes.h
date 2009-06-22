
#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwio/ntfileapi.h"
#include <lwio/win32fileapi.h>
#include <krb5/krb5.h>
#include "lwnet.h"
#include <stdlib.h>

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096


NTSTATUS
CopyFileFromNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
CopyDirFromNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
CopyFileToNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
CopyDirToNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );
