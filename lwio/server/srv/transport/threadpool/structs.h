#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_SRV_SOCKET
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    int fd;
    struct sockaddr_in cliaddr;
} LWIO_SRV_SOCKET, *PLWIO_SRV_SOCKET;

typedef struct _LWIO_SRV_LISTENER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PLW_THREAD_POOL pPool;
    PLW_TASK pTask;
    PLW_TASK_GROUP pTaskGroup;
    int listenFd;

    SRV_PROPERTIES serverProperties;

    // Invariant
    // Not owned
    HANDLE                     hPacketAllocator;
    HANDLE                     hGssContext;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList;
} LWIO_SRV_LISTENER_CONTEXT, *PLWIO_SRV_LISTENER_CONTEXT;

typedef struct _LWIO_SRV_LISTENER
{
    LWIO_SRV_LISTENER_CONTEXT context;

} LWIO_SRV_LISTENER, *PLWIO_SRV_LISTENER;

typedef struct _LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG
{
    BOOLEAN bEnableSigning;
    BOOLEAN bRequireSigning;

} LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG, *PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG;

typedef struct _LWIO_SRV_THREADPOOL_TRANSPORT_GLOBALS
{
    pthread_mutex_t              mutex;

    SRV_TRANSPORT_FUNCTION_TABLE fnTable;

    PSMB_PROD_CONS_QUEUE         pWorkQueue;

    LWIO_SRV_LISTENER            listener;

    PLWIO_PACKET_ALLOCATOR       hPacketAllocator;

    PLWIO_SRV_SHARE_ENTRY_LIST   pShareList;

    LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG config;

} LWIO_SRV_THREADPOOL_TRANSPORT_GLOBALS, *PLWIO_SRV_THREADPOOL_TRANSPORT_GLOBALS;

#endif /* __STRUCTS_H__ */
