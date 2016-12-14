/* ex: set shiftwidth=4 softtabstop=4 expandtab: */
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dce/nbase.h>
#include <dce/dce_error.h>

#ifdef _WIN32 /* needed for exit() prototype */
#include <process.h>
#endif
#include "misc.h"


void 
chk_dce_err(
    error_status_t ecode,
    char * where,
    char * why,
    unsigned int fatal
    )
{
    dce_error_string_t errstr;
    int error_status;                           
  
    if (ecode != error_status_ok)
    {
        dce_error_inq_text(ecode, (unsigned char *) errstr, &error_status); 
        if (error_status == error_status_ok)
            printf("ERROR.  where = <%s> why = <%s> error code = 0x%x"
                   "reason = <%s>\n",
                   where, why, ecode, errstr);
        else
            printf("ERROR.  where = <%s> why = <%s> error code = 0x%x\n",
                   where, why, ecode);
       
        if (fatal) exit(1);
    }
}
