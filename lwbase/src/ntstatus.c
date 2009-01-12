#include "includes.h"

typedef struct _TABLE_ENTRY
{
    NTSTATUS code;
    int unixErrno;
    PCSTR pszSymbolicName;
    PCSTR pszDescription;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
LwNtLookupCode(
    NTSTATUS code
    );

#define NTSTATUS_CODE(code, err, desc) { code, err, #code, desc },
TABLE_ENTRY codeTable[] =
{
#include "ntstatus-table.h"
    {-1, -1, NULL, NULL}
};
#undef NTSTATUS_CODE

PCSTR
LwNtStatusToSymbolicName(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        return pEntry->pszSymbolicName;
    }
    else
    {
        return "UNKNOWN";
    }
}

PCSTR
LwNtStatusToDescription(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        if (pEntry->pszDescription)
        {
            return pEntry->pszDescription;
        }
        else
        {
            return "No description available";
        }
    }
    else
    {
        return "Unknown error";
    }
}

int
LwNtStatusToUnixErrno(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        return pEntry->unixErrno;
    }
    else
    {
        return -1;
    }
}

static
PTABLE_ENTRY
LwNtLookupCode(
    NTSTATUS code
    )
{
    ULONG index;

    for (index = 0; index < sizeof(codeTable) / sizeof(*codeTable); index++)
    {
        if (codeTable[index].code == code)
        {
            return &codeTable[index];
        }
    }

    return NULL;
}
