/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "domainjoin.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
 
#if defined(_AIX)

    if (argc < 2) {
      printf("Usage: test_sec_user <config file path>\n");
      exit(1);
    }

    ceError = ConfigureUserSecurity(argv[1]);
    BAIL_ON_CENTERIS_ERROR(ceError);

#else

    ceError = 1;

    printf("This test is valid only on AIX\n");

#endif

error:

    if (CENTERROR_IS_OK(ceError))
      printf("Success\n");
    else
      printf("Failed\n");

    return(ceError);
}
