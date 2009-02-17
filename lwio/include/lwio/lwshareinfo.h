typedef union _SHARE_INFO_UNION
{
    PSHARE_INFO_0   p0;
    PSHARE_INFO_1   p1;
    PSHARE_INFO_501 p501;
    PSHARE_INFO_502 p502;
} SHARE_INFO_UNION, *PSHARE_INFO_UNION;


typedef struct _SHARE_INFO_ADD_PARAMS
{
    DWORD dwInfoLevel;
    SHARE_INFO_UNION info;
} SHARE_INFO_ADD_PARAMS, *PSHARE_INFO_ADD_PARAMS;

typedef struct _SHARE_INFO_DELETE_PARAMS
{
    PWSTR servername;
    PWSTR netname;
    DWORD reserved;
}SHARE_INFO_DELETE_PARAMS, *PSHARE_INFO_DELETE_PARAMS;

typedef struct _SHARE_INFO_SETINFO_PARAMS
{
    PWSTR servername;
    PWSTR netname;
    DWORD level;
    SHARE_INFO_UNION info;
}SHARE_INFO_SETINFO_PARAMS, *PSHARE_INFO_SETINFO_PARAMS;


LW_NTSTATUS
LwShareInfoUnmarshalAddParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_ADD_PARAMS* ppParams
    );

LW_NTSTATUS
LwShareInfoMarshalDeleteParameters(
    PSHARE_INFO_ADD_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );

LW_NTSTATUS
LwShareInfoUnmarshalDeleteParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_DELETE_PARAMS* ppParams
    );


