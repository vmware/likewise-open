#include "domainjoin.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
 
    ceError = ConfigureNameServiceSwitch();
    BAIL_ON_CENTERIS_ERROR(ceError);
 
error:

    return(ceError);
}
