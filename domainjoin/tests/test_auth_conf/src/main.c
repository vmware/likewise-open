#include "domainjoin.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (argc <= 1) {
       printf("Usage: test_auth_conf <workgroup name>\n");
       exit(0);
    }

    ceError = SetWorkgroup(NULL, argv[1]);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return(ceError);
}
