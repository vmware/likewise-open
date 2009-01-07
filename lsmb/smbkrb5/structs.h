#ifndef __STRUCTS_H__
#define __STRUCTS_H__


/*
   Based on RFC2478

   MechType ::= OBJECT IDENTIFIER

   MechTypeList ::= SEQUENCE OF MechType

   ContextFlags ::= BIT STRING
                    {
                         delegFlag  (0),
                         mutualFlag (1),
                         replayFlag (2),
                         sequenceFlag (3),
                         anonFlag     (4),
                         confFlag     (5),
                         integFlag    (6)
                    }

   NegTokenInit ::= SEQUENCE OF
                    {
                         mechTypes   [0] MechTypeList OPTIONAL
                         reqFlags    [1] ContextFlags OPTIONAL
                         mechToken   [2] OCTET STRING OPTIONAL
                         mechListMIC [3] OCTET STRING OPTIONAL
                    }

 */

typedef struct __OID
{
    uint32_t dwLength;
    uint8_t* pIdList;
} OID, *POID;

typedef OID MECHTYPE, *PMECHTYPE;

typedef struct __MECHTYPELIST
{
    uint32_t dwLength;
    PMECHTYPE pMechTypeArray;
} MECHTYPELIST, *PMECHTYPELIST;

typedef struct __OCTET_STRING
{
    uint32_t dwLength;
    PBYTE    pData;
} OCTET_STRING, *POCTET_STRING;

typedef struct __CONTEXT_FLAGS
{
    uint32_t delegFlag    : 1;
    uint32_t mutualFlag   : 1;
    uint32_t replayFlag   : 1;
    uint32_t sequenceFlag : 1;
    uint32_t anonFlag     : 1;
    uint32_t confFlag     : 1;
    uint32_t integFlag    : 1;
} CONTEXT_FLAGS, *PCONTEXT_FLAGS;

typedef struct __NEGTOKENINIT
{
    PMECHTYPELIST pMechTypeList;
    PCONTEXT_FLAGS pContextFlags;
    POCTET_STRING pMechToken;
    POCTET_STRING pMechListMIC;
} NEGTOKENINIT, *PNEGTOKENINIT;

typedef struct __SMB_GSS_SEC_CONTEXT
{
    SMB_GSS_SEC_CONTEXT_STATE state;
    PCtxtHandle               pGSSContext;
    gss_name_t                target_name;
    PSTR                      pszTargetName;
} SMB_GSS_SEC_CONTEXT, *PSMB_GSS_SEC_CONTEXT;

#endif

