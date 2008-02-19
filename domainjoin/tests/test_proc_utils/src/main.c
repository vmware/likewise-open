/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "domainjoin.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PROCBUFFER procBuf;
    PSTR ppArgs[] = {"/bin/date", NULL};
    LONG lExitCode = 0;
 
    ceError = DJSpawnProcess("/bin/date", ppArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    printf("Child process id: %d\n", pProcInfo->pid); 
    printf("<Begin output>\n");
    do {
      procBuf.bEndOfFile = FALSE;
      ceError = DJReadData(pProcInfo, &procBuf);
      BAIL_ON_CENTERIS_ERROR(ceError);
      if (procBuf.dwOutBytesRead) {
         if (write(STDOUT_FILENO, procBuf.szOutBuf, procBuf.dwOutBytesRead) < 0) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
         }
      }
      if (procBuf.dwErrBytesRead) {
         if (write(STDERR_FILENO, procBuf.szErrBuf, procBuf.dwErrBytesRead) < 0) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
         }
      }
    } while (!procBuf.bEndOfFile);
    printf("<End output>\n");

    ceError = DJGetProcessStatus(pProcInfo, &lExitCode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    printf("Exit status: %ld\n", lExitCode);
 
error:

    if (pProcInfo) {
       FreeProcInfo(pProcInfo);
    }

    return(ceError);
}
