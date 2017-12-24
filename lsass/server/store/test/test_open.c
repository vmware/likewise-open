#include "includes.h"

int main(void)
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;

    printf("test_main: called\n");
    dwError = DirectoryOpen(&hDirectory);
    if (dwError)
    {
        printf("DirectoryOpen: failed %d (0x%x)\n", dwError, dwError);
        return 1;
    }

    return 0;
}
