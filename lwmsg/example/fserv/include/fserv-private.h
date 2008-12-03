#ifndef __FSERV_PRIVATE_H__
#define __FSERV_PRIVATE_H__

#include "fserv.h"
#include "protocol.h"

struct FServ
{
    LWMsgAssoc* assoc;
};

struct FServFile
{
    FServ* fserv;
    FileHandle* handle;
};

#endif
