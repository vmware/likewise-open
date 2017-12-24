#include "includes.h"

void SamDbDebugCall(char *log, char *file, int line)
{
    FILE *fp = fopen(log, "a+");
    if (!fp)
    {
        return;
    }
    
    fprintf(fp, "SamDbDebugCall: FILE=%s LINE=%d\n", file, line);
    fclose(fp);
}
