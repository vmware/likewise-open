#include "includes.h"

int
main(
    int argc,
    char *argv[])
{
    DWORD dwError = 0;

    dwError = ReadMachineAccountSqlite(argv[1], argv[2]);

    if ( dwError )
        return 1;
    else
        return 0;
}
