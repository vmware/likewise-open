#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include <dce/dce_error.h>
#include <dce/smb.h>
#include <wc16str.h>
#include <lw/base.h>
#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwio/lwio.h>

#include <lw/srvsvc.h>
#include <lwrpc/LMcreds.h>

#include "../client/srvsvc_util.h"

#include "test.h"
#include "params.h"
