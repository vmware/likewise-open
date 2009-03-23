/* This file is a copy of excerpt from generated srvsvc_h.h header.
   It has to be in sync with original idl file.
 */

#ifndef _SRVSVC_STUB_H_
#define _SRVSVC_STUB_H_

extern void _srvsvc_Function0(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function1(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function2(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function3(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function4(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function5(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function6(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function7(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
typedef struct  {
uint32 count;
CONNECTION_INFO_0 *array;
} srvsvc_NetConnCtr0;
typedef struct  {
uint32 count;
CONNECTION_INFO_1 *array;
} srvsvc_NetConnCtr1;
typedef union  {
/* case(s): 0 */
srvsvc_NetConnCtr0 *ctr0;
/* case(s): 1 */
srvsvc_NetConnCtr1 *ctr1;
/* case(s): default */
/* Empty arm */
} srvsvc_NetConnCtr;
extern NET_API_STATUS _NetrConnectionEnum(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *qualifier,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetConnCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
#endif
);
typedef struct  {
uint32 count;
FILE_INFO_2 *array;
} srvsvc_NetFileCtr2;
typedef struct  {
uint32 count;
FILE_INFO_3 *array;
} srvsvc_NetFileCtr3;
typedef union  {
/* case(s): 2 */
srvsvc_NetFileCtr2 *ctr2;
/* case(s): 3 */
srvsvc_NetFileCtr3 *ctr3;
/* case(s): default */
/* Empty arm */
} srvsvc_NetFileCtr;
extern NET_API_STATUS _NetrFileEnum(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *basepath,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetFileCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
#endif
);
typedef union  {
/* case(s): 2 */
FILE_INFO_2 *info2;
/* case(s): 3 */
FILE_INFO_3 *info3;
/* case(s): default */
/* Empty arm */
} srvsvc_NetFileInfo;
extern NET_API_STATUS _NetrFileGetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetFileInfo *info
#endif
);
extern NET_API_STATUS _NetrFileClose(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid
#endif
);
typedef struct  {
uint32 count;
SESSION_INFO_0 *array;
} srvsvc_NetSessCtr0;
typedef struct  {
uint32 count;
SESSION_INFO_1 *array;
} srvsvc_NetSessCtr1;
typedef struct  {
uint32 count;
SESSION_INFO_2 *array;
} srvsvc_NetSessCtr2;
typedef struct  {
uint32 count;
SESSION_INFO_10 *array;
} srvsvc_NetSessCtr10;
typedef struct  {
uint32 count;
SESSION_INFO_502 *array;
} srvsvc_NetSessCtr502;
typedef union  {
/* case(s): 0 */
srvsvc_NetSessCtr0 *ctr0;
/* case(s): 1 */
srvsvc_NetSessCtr1 *ctr1;
/* case(s): 2 */
srvsvc_NetSessCtr2 *ctr2;
/* case(s): 10 */
srvsvc_NetSessCtr10 *ctr10;
/* case(s): 502 */
srvsvc_NetSessCtr502 *ctr502;
/* case(s): default */
/* Empty arm */
} srvsvc_NetSessCtr;
extern NET_API_STATUS _NetrSessionEnum(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *unc_client_name,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetSessCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
#endif
);
extern void _srvsvc_FunctionD(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
typedef struct _SHARE_INFO_502_I {
wchar16_t *shi502_netname;
uint32 shi502_type;
wchar16_t *shi502_remark;
uint32 shi502_permissions;
uint32 shi502_max_uses;
uint32 shi502_current_uses;
wchar16_t *shi502_path;
wchar16_t *shi502_password;
uint32 shi502_reserved;
uint8 *shi502_security_descriptor;
} SHARE_INFO_502_I;
typedef SHARE_INFO_502_I *PSHARE_INFO_502_I;
typedef SHARE_INFO_502_I *LPSHARE_INFO_502_I;
typedef struct _SHARE_INFO_1501_I {
uint32 shi1501_reserved;
uint8 *shi1501_security_descriptor;
} SHARE_INFO_1501_I;
typedef SHARE_INFO_1501_I *PSHARE_INFO_1501_I;
typedef SHARE_INFO_1501_I *LPSHARE_INFO_1501_I;
typedef union  {
/* case(s): 0 */
SHARE_INFO_0 *info0;
/* case(s): 1 */
SHARE_INFO_1 *info1;
/* case(s): 2 */
SHARE_INFO_2 *info2;
/* case(s): 501 */
SHARE_INFO_501 *info501;
/* case(s): 502 */
SHARE_INFO_502_I *info502;
/* case(s): 1004 */
SHARE_INFO_1004 *info1004;
/* case(s): 1005 */
SHARE_INFO_1005 *info1005;
/* case(s): 1006 */
SHARE_INFO_1006 *info1006;
/* case(s): 1501 */
SHARE_INFO_1501_I *info1501;
/* case(s): default */
/* Empty arm */
} srvsvc_NetShareInfo;
extern NET_API_STATUS _NetrShareAdd(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ uint32 *parm_error
#endif
);
typedef struct  {
uint32 count;
SHARE_INFO_0 *array;
} srvsvc_NetShareCtr0;
typedef struct  {
uint32 count;
SHARE_INFO_1 *array;
} srvsvc_NetShareCtr1;
typedef struct  {
uint32 count;
SHARE_INFO_2 *array;
} srvsvc_NetShareCtr2;
typedef struct  {
uint32 count;
SHARE_INFO_501 *array;
} srvsvc_NetShareCtr501;
typedef struct  {
uint32 count;
SHARE_INFO_502_I *array;
} srvsvc_NetShareCtr502;
typedef union  {
/* case(s): 0 */
srvsvc_NetShareCtr0 *ctr0;
/* case(s): 1 */
srvsvc_NetShareCtr1 *ctr1;
/* case(s): 2 */
srvsvc_NetShareCtr2 *ctr2;
/* case(s): 501 */
srvsvc_NetShareCtr501 *ctr501;
/* case(s): 502 */
srvsvc_NetShareCtr502 *ctr502;
/* case(s): default */
/* Empty arm */
} srvsvc_NetShareCtr;
extern NET_API_STATUS _NetrShareEnum(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetShareCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
#endif
);
extern NET_API_STATUS _NetrShareGetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetShareInfo *info
#endif
);
extern NET_API_STATUS _NetrShareSetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ uint32 *parm_error
#endif
);
extern NET_API_STATUS _NetrShareDel(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 reserved
#endif
);
extern void _srvsvc_Function13(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function14(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
typedef union  {
/* case(s): 100 */
SERVER_INFO_100 *info100;
/* case(s): 101 */
SERVER_INFO_101 *info101;
/* case(s): 102 */
SERVER_INFO_102 *info102;
/* case(s): 402 */
SERVER_INFO_402 *info402;
/* case(s): 403 */
SERVER_INFO_403 *info403;
/* case(s): 502 */
SERVER_INFO_502 *info502;
/* case(s): 503 */
SERVER_INFO_503 *info503;
/* case(s): 599 */
SERVER_INFO_599 *info599;
/* case(s): 1005 */
SERVER_INFO_1005 *info1005;
/* case(s): 1010 */
SERVER_INFO_1010 *info1010;
/* case(s): 1016 */
SERVER_INFO_1016 *info1016;
/* case(s): 1017 */
SERVER_INFO_1017 *info1017;
/* case(s): 1018 */
SERVER_INFO_1018 *info1018;
/* case(s): 1107 */
SERVER_INFO_1107 *info1107;
/* case(s): 1501 */
SERVER_INFO_1501 *info1501;
/* case(s): 1502 */
SERVER_INFO_1502 *info1502;
/* case(s): 1503 */
SERVER_INFO_1503 *info1503;
/* case(s): 1506 */
SERVER_INFO_1506 *info1506;
/* case(s): 1509 */
SERVER_INFO_1509 *info1509;
/* case(s): 1510 */
SERVER_INFO_1510 *info1510;
/* case(s): 1511 */
SERVER_INFO_1511 *info1511;
/* case(s): 1512 */
SERVER_INFO_1512 *info1512;
/* case(s): 1513 */
SERVER_INFO_1513 *info1513;
/* case(s): 1514 */
SERVER_INFO_1514 *info1514;
/* case(s): 1515 */
SERVER_INFO_1515 *info1515;
/* case(s): 1516 */
SERVER_INFO_1516 *info1516;
/* case(s): 1518 */
SERVER_INFO_1518 *info1518;
/* case(s): 1520 */
SERVER_INFO_1520 *info1520;
/* case(s): 1521 */
SERVER_INFO_1521 *info1521;
/* case(s): 1522 */
SERVER_INFO_1522 *info1522;
/* case(s): 1523 */
SERVER_INFO_1523 *info1523;
/* case(s): 1524 */
SERVER_INFO_1524 *info1524;
/* case(s): 1525 */
SERVER_INFO_1525 *info1525;
/* case(s): 1528 */
SERVER_INFO_1528 *info1528;
/* case(s): 1529 */
SERVER_INFO_1529 *info1529;
/* case(s): 1530 */
SERVER_INFO_1530 *info1530;
/* case(s): 1533 */
SERVER_INFO_1533 *info1533;
/* case(s): 1534 */
SERVER_INFO_1534 *info1534;
/* case(s): 1535 */
SERVER_INFO_1535 *info1535;
/* case(s): 1536 */
SERVER_INFO_1536 *info1536;
/* case(s): 1537 */
SERVER_INFO_1537 *info1537;
/* case(s): 1538 */
SERVER_INFO_1538 *info1538;
/* case(s): 1539 */
SERVER_INFO_1539 *info1539;
/* case(s): 1540 */
SERVER_INFO_1540 *info1540;
/* case(s): 1541 */
SERVER_INFO_1541 *info1541;
/* case(s): 1542 */
SERVER_INFO_1542 *info1542;
/* case(s): 1543 */
SERVER_INFO_1543 *info1543;
/* case(s): 1544 */
SERVER_INFO_1544 *info1544;
/* case(s): 1545 */
SERVER_INFO_1545 *info1545;
/* case(s): 1546 */
SERVER_INFO_1546 *info1546;
/* case(s): 1547 */
SERVER_INFO_1547 *info1547;
/* case(s): 1548 */
SERVER_INFO_1548 *info1548;
/* case(s): 1549 */
SERVER_INFO_1549 *info1549;
/* case(s): 1550 */
SERVER_INFO_1550 *info1550;
/* case(s): 1552 */
SERVER_INFO_1552 *info1552;
/* case(s): 1553 */
SERVER_INFO_1553 *info1553;
/* case(s): 1554 */
SERVER_INFO_1554 *info1554;
/* case(s): 1555 */
SERVER_INFO_1555 *info1555;
/* case(s): 1556 */
SERVER_INFO_1556 *info1556;
/* case(s): default */
/* Empty arm */
} srvsvc_NetSrvInfo;
extern NET_API_STATUS _NetrServerGetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetSrvInfo *info
#endif
);
extern NET_API_STATUS _NetrServerSetInfo(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetSrvInfo info,
    /* [in, out] */ uint32 *parm_error
#endif
);
extern void _srvsvc_Function17(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function18(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function19(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function1a(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function1b(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NET_API_STATUS _NetrRemoteTOD(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [out] */ TIME_OF_DAY_INFO **info
#endif
);
extern void _srvsvc_Function1d(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function1e(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function1f(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern void _srvsvc_Function20(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle
#endif
);
extern NET_API_STATUS _NetrNameValidate(
#ifdef IDL_PROTOTYPES
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *name,
    /* [in] */ uint32 type,
    /* [in] */ uint32 flags
#endif
);


extern rpc_if_handle_t srvsvc_v3_0_c_ifspec;
extern rpc_if_handle_t srvsvc_v3_0_s_ifspec;

#endif /* _SRVSVC_STUB_H_ */
