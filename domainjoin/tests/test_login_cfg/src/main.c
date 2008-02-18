#include "domainjoin.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
 
#if defined (_AIX)

    if (argc < 2) {
      printf("Usage: test_login_cfg <config file path>\n");
      exit(1);
    }
    
    ceError = DJFixLoginConfigFile(argv[1]);
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
