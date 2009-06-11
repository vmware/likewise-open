#ifndef __READER_H__
#define __READER_H__

NTSTATUS
SrvSocketReaderInit(
    PSMB_PROD_CONS_QUEUE   pWorkQueue,
    PLWIO_SRV_SOCKET_READER pReader
    );

ULONG
SrvSocketReaderGetCount(
    PLWIO_SRV_SOCKET_READER pReader
    );

BOOLEAN
SrvSocketReaderIsActive(
    PLWIO_SRV_SOCKET_READER pReader
    );

NTSTATUS
SrvSocketReaderEnqueueConnection(
    PLWIO_SRV_SOCKET_READER pReader,
    PLWIO_SRV_CONNECTION    pConnection
    );

NTSTATUS
SrvSocketReaderFreeContents(
    PLWIO_SRV_SOCKET_READER pReader
    );

#endif /* __READER_H__ */
