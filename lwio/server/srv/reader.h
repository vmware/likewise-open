#ifndef __READER_H__
#define __READER_H__

NTSTATUS
SrvSocketReaderInit(
    PSMB_SRV_SOCKET_READER pReader
    );

ULONG
SrvSocketReaderGetCount(
    PSMB_SRV_SOCKET_READER pReader
    );

NTSTATUS
SrvSocketReaderEnqueueConnection(
    PSMB_SRV_SOCKET_READER pReader,
    PSMB_SRV_CONNECTION    pConnection
    );

NTSTATUS
SrvSocketReaderFreeContents(
    PSMB_SRV_SOCKET_READER pReader
    );

#endif /* __READER_H__ */
