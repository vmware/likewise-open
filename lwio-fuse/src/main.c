/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"

#define LWIO_OPT_KEY(t, p, v) { t, offsetof(IO_FUSE_CONTEXT, p), v }

enum
{
    KEY_VERSION,
    KEY_HELP
};

static struct fuse_opt lwio_opts[] =
{
    LWIO_OPT_KEY("--driver %s", pszDriver, 0),
    LWIO_OPT_KEY("--server %s", pszServer, 0),
    LWIO_OPT_KEY("--share %s", pszShare, 0),
    LWIO_OPT_KEY("-h", bHelp, 1),
    LWIO_OPT_KEY("--help", bHelp, 1),
    FUSE_OPT_END
};

static
void
show_help()
{
    printf("lwio-fuse-mount: mount lwio path onto filesystem\n"
           "\n"
           "Usage: lwio-fuse-mount [--driver name] --server host --share sharename mount_path\n"
           "\n"
           "Options:\n"
           "\n"
           "    --driver name             Specify custom driver (default: rdr)\n"
           "    --server host             Specify remote host\n"
           "    --share  sharename        Specify share on remote host to mount\n"
           "\n");
}

int
main(int argc,
     char** argv
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    static PCSTR pszDriver = "rdr";
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    status = RTL_ALLOCATE(&pFuseContext, IO_FUSE_CONTEXT, sizeof(*pFuseContext));
    BAIL_ON_NT_STATUS(status);

    if (fuse_opt_parse(&args, pFuseContext, lwio_opts, NULL) == -1)
    {
        goto error;
    }

    if (pFuseContext->bHelp)
    {
        show_help();
        return 0;
    }

    if (!pFuseContext->pszDriver)
    {
        status = LwRtlCStringDuplicate(
            &pFuseContext->pszDriver,
            pszDriver);
        BAIL_ON_NT_STATUS(status);
    }

    if (!pFuseContext->pszServer)
    {
        printf("Error: No server specified\n");
        goto error;
    }
    
    if (!pFuseContext->pszShare)
    {
        printf("Error: No share specified\n");
        goto error;
    }

    pFuseContext->ownerUid = geteuid();
    pFuseContext->ownerGid = getegid();
  
    return fuse_main(args.argc, args.argv, LwIoFuseGetOperationsTable(), pFuseContext);

error:

    return 1;
}
