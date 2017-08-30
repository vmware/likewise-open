#include "includes.h"

char *SamDbDebugWC16ToC(PWSTR pwszValue)
{
    NTSTATUS ntStatus = 0;
    PSTR pszValue = NULL;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszValue, pwszValue);
    if (ntStatus)
    {
        pszValue = NULL;
    }
    return pszValue;
}

void SamDbDebugCall(char *log, const char *func, char *file, int line, char *optional_msg, char *optional_data)
{
    FILE *fp = fopen(log, "a+");
    time_t t = time(NULL);
    char ctimebuf[32];

    if (!fp)
    {
        return;
    }
    
    fprintf(fp, "%.24s SamDbDebugCall: FUNCTION=%s FILE=%s LINE=%d", ctime_r(&t, ctimebuf), func, file, line);
    if (optional_msg)
    {
        fprintf(fp, " %s", optional_msg);
    }
    if (optional_data)
    {
        fprintf(fp, "'%s'", optional_data);
    }
    fprintf(fp, "\n");

    fclose(fp);
}
