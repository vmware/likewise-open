#ifndef __PRODCONS_H__
#define __PRODCONS_H__

NTSTATUS
SrvProdConsInit(
    PSMB_PROD_CONS_QUEUE pQueue
    );

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

#endif /* __PRODCONS_H__ */
