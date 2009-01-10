#ifndef __READER_H__
#define __READER_H__

NTSTATUS
SrvSocketReaderInit(
    PSMB_SRV_SOCKET_READER pReader
    );

NTSTATUS
SrvSocketReaderFreeContents(
    PSMB_SRV_SOCKET_READER pReader
    );

#endif /* __READER_H__ */
