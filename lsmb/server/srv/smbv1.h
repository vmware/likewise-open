#ifndef __SMBV1_H__
#define __SMBV1_H__

DWORD
SMBSrvInitialize_V1(
    PCSTR pszConfigFilePath
    );

DWORD
SMBSrvProcessRequest_V1(
    PSMB_PACKET pRequest,
    PSMB_PACKET pResponse
    );

DWORD
SMBSrvShutdown_V1(
    VOID
    );

#endif /* __SMBV1_H__ */
