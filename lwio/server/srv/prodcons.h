#ifndef __PRODCONS_H__
#define __PRODCONS_H__

NTSTATUS
SrvProdConsInit(
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PSMB_PROD_CONS_QUEUE*         ppQueue
    );

NTSTATUS
SrvProdConsInitContents(
    PSMB_PROD_CONS_QUEUE          pQueue,
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
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
SrvProdConsFree(
    PSMB_PROD_CONS_QUEUE pQueue
    );

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

#endif /* __PRODCONS_H__ */
