#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_PACKET_ALLOCATOR
{
    pthread_mutex_t mutex;
    pthread_mutex_t* pMutex;

    PSMB_STACK pFreeBufferStack;
    uint32_t   freeBufferCount;
    /* Current size of buffer allocator */
    size_t     freeBufferLen;

    PSMB_STACK pFreePacketStack;
    uint32_t   freePacketCount;

    uint32_t   dwNumMaxPackets;

} LWIO_PACKET_ALLOCATOR, *PLWIO_PACKET_ALLOCATOR;

#endif /* __STRUCTS_H__ */
