#include "config.h"
#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <lwio/lwio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <wc16str.h>

#define PAYLOAD "Hello, World!\r\n"

typedef struct _LOAD_THREAD
{
    pthread_t Thread;
    ULONG ulNumber;
} LOAD_THREAD, *PLOAD_THREAD;

typedef struct _LOAD_FILE
{
    IO_FILE_NAME Filename;
    IO_FILE_HANDLE hHandle;
} LOAD_FILE, *PLOAD_FILE;

struct
{
    ULONG ulThreadCount;
    ULONG ulConnectionsPerThread;
    ULONG ulIterations;
    PCSTR pszServer;
    PCSTR pszShare;
    BOOLEAN volatile bStart;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
} gState =
{
    .ulThreadCount = 100,
    .ulConnectionsPerThread = 100,
    .ulIterations = 10,
    .bStart = FALSE,
    .Lock = PTHREAD_MUTEX_INITIALIZER,
    .Event = PTHREAD_COND_INITIALIZER
};

static
PVOID
LoadThread(
    PVOID pData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOAD_THREAD pThread = (PLOAD_THREAD) pData;
    ULONG ulIter = 0;
    ULONG ulFile = 0;
    PLOAD_FILE pFiles = NULL;
    PLOAD_FILE pFile = NULL;
    static const CHAR szPayload[] = PAYLOAD;
    CHAR szCompare[sizeof(szPayload)];
    IO_STATUS_BLOCK ioStatus = {0};
    LONG64 llOffset = 0;
    CHAR szHostname[256] = {0};

    if (gethostname(szHostname, sizeof(szHostname) -1) != 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pFiles, LOAD_FILE, sizeof(*pFiles) * gState.ulConnectionsPerThread);
    GOTO_ERROR_ON_STATUS(status);

    printf("[%u] Creating start up environment...\n",
           pThread->ulNumber);

    for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
    {
        pFile = &pFiles[ulFile];

        status = LwRtlWC16StringAllocatePrintfW(
            &pFile->Filename.FileName,
            L"/rdr/%s@%u/%s/test-load-%s-%u.txt",
            gState.pszServer,
            pThread->ulNumber * gState.ulThreadCount + ulFile,
            gState.pszShare,
            szHostname,
            pThread->ulNumber * gState.ulThreadCount + ulFile);
        GOTO_ERROR_ON_STATUS(status);
    }

    pthread_mutex_lock(&gState.Lock);
    while (!gState.bStart)
    {
        pthread_cond_wait(&gState.Event, &gState.Lock);
    }
    pthread_mutex_unlock(&gState.Lock);


    for (ulIter = 0; ulIter < gState.ulIterations; ulIter++)
    {
        printf("[%u] Starting iteration %d of %d...\n",
               pThread->ulNumber,
               ulIter + 1,
               gState.ulIterations);

        /* Pass 1 -- open files for writing */

        printf("[%u] Opening %u files for writing...\n",
               pThread->ulNumber,
               gState.ulConnectionsPerThread);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            status = LwNtCreateFile(
                &pFile->hHandle,       /* File handle */
                NULL,                  /* Async control block */
                &ioStatus,             /* IO status block */
                &pFile->Filename,      /* Filename */
                NULL,                  /* Security descriptor */
                NULL,                  /* Security QOS */
                FILE_GENERIC_WRITE,    /* Desired access mask */
                0,                     /* Allocation size */
                0,                     /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,     /* Share access */
                FILE_OVERWRITE_IF,     /* Create disposition */
                0,                     /* Create options */
                NULL,                  /* EA buffer */
                0,                     /* EA length */
                NULL);                 /* ECP list */
            GOTO_ERROR_ON_STATUS(status);
        }

        /* Pass 2 -- write payload into each file */

        printf("[%u] Writing to files...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            llOffset = 0;

            status = LwNtWriteFile(
                pFile->hHandle, /* File handle */
                NULL, /* Async control block */
                &ioStatus, /* IO status block */
                (PVOID) szPayload, /* Buffer */
                sizeof(szPayload), /* Buffer size */
                &llOffset, /* File offset */
                NULL); /* Key */
            GOTO_ERROR_ON_STATUS(status);
        }

        /* Pass 3 -- reopen each file for reading */

        printf("[%u] Reopening files for reading...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            status = LwNtCloseFile(pFile->hHandle);
            GOTO_ERROR_ON_STATUS(status);

            pFile->hHandle = NULL;

            status = LwNtCreateFile(
                &pFile->hHandle,       /* File handle */
                NULL,                  /* Async control block */
                &ioStatus,             /* IO status block */
                &pFile->Filename,      /* Filename */
                NULL,                  /* Security descriptor */
                NULL,                  /* Security QOS */
                FILE_GENERIC_READ |
                DELETE,                /* Desired access mask */
                0,                     /* Allocation size */
                0,                     /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,     /* Share access */
                FILE_OPEN,             /* Create disposition */
                FILE_DELETE_ON_CLOSE,  /* Create options */
                NULL,                  /* EA buffer */
                0,                     /* EA length */
                NULL);                 /* ECP list */
            GOTO_ERROR_ON_STATUS(status);
        }

        /* Pass 4 -- read back each payload and compare */

        printf("[%u] Reading files...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            llOffset = 0;

            status = LwNtReadFile(
                pFile->hHandle, /* File handle */
                NULL, /* Async control block */
                &ioStatus, /* IO status block */
                szCompare, /* Buffer */
                sizeof(szCompare), /* Buffer size */
                &llOffset, /* File offset */
                NULL); /* Key */
            GOTO_ERROR_ON_STATUS(status);

            if (ioStatus.BytesTransferred != sizeof(szCompare) ||
                memcmp(szCompare, szPayload, sizeof(szCompare)))
            {
                status = STATUS_UNSUCCESSFUL;
                GOTO_ERROR_ON_STATUS(status);
            }
        }

        /* Pass 5 -- close handles */

        printf("[%u] Closing files...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            status = LwNtCloseFile(pFile->hHandle);
            GOTO_ERROR_ON_STATUS(status);
        }
    }

error:

    if (status != STATUS_SUCCESS)
    {
        fprintf(stderr, "Error: %s (%x)\n", LwNtStatusToName(status), status);
        abort();
    }


    return NULL;
}

static
NTSTATUS
Run(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOAD_THREAD pThreads = NULL;
    PLOAD_THREAD pThread = NULL;
    ULONG ulThread = 0;

    status = RTL_ALLOCATE(&pThreads, LOAD_THREAD, sizeof(*pThreads) * gState.ulThreadCount);
    GOTO_ERROR_ON_STATUS(status);

    for (ulThread = 0; ulThread < gState.ulThreadCount; ulThread++)
    {
        pThread = &pThreads[ulThread];

        pThread->ulNumber = ulThread;

        status = LwErrnoToNtStatus(
            pthread_create(
                &pThread->Thread,
                NULL,
                LoadThread,
                pThread));
        GOTO_ERROR_ON_STATUS(status);
    }

    pthread_mutex_lock(&gState.Lock);
    gState.bStart = TRUE;
    pthread_cond_broadcast(&gState.Event);
    pthread_mutex_unlock(&gState.Lock);


    for (ulThread = 0; ulThread < gState.ulThreadCount; ulThread++)
    {
        pThread = &pThreads[ulThread];

        status = LwErrnoToNtStatus(pthread_join(pThread->Thread, NULL));
        GOTO_ERROR_ON_STATUS(status);
    }

error:

    RTL_FREE(&pThreads);

    return status;
}

static
VOID
Usage(
    PCSTR pszProgram
    )
{
    printf("Usage: %s [ options ... ] server share\n", pszProgram);
}

static
VOID
Help(
    PCSTR pszProgram
    )
{
    Usage(pszProgram);

    printf(
        "\n"
        "Options:\n"
        "\n"
        "  --help                            Show this help\n"
        "  --iterations count                Number of iterations of open-write-read-close cycle\n"
        "  --threads count                   Number of threads to spawn\n"
        "  --connections count               Number of connections to create per thread\n");
}

static
VOID
ParseArgs(
    int argc,
    char** ppszArgv
    )
{
    int i = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(ppszArgv[i], "--help"))
        {
            Help(ppszArgv[0]);
            exit(0);
        }
        else if (!strcmp(ppszArgv[i], "--iterations"))
        {
            if (i + i == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulIterations = atoi(ppszArgv[++i]);
        }
        else if (!strcmp(ppszArgv[i], "--threads"))
        {
            if (i + i == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulThreadCount = atoi(ppszArgv[++i]);
        }
        else if (!strcmp(ppszArgv[i], "--connections"))
        {
            if (i + i == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulConnectionsPerThread = atoi(ppszArgv[++i]);
        }
        else
        {
            if (i + 1 >= argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }

            gState.pszServer = ppszArgv[i];
            gState.pszShare = ppszArgv[++i];

            return;
        }
    }

    if (!gState.pszServer)
    {
        Usage(ppszArgv[0]);
        exit(1);
    }
}

int
main(
    int argc,
    char** ppszArgv
    )
{
    ParseArgs(argc, ppszArgv);

    return Run();
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

