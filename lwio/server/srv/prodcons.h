#ifndef __PRODCONS_H__
#define __PRODCONS_H__

NTSTATUS
SrvProdConsInit(
    PSMB_PROD_CONS_QUEUE pQueue
    );

NTSTATUS
SrvProdConsEnqueue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    );

NTSTATUS
SrvProdConsDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    );

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

#endif /* __PRODCONS_H__ */
