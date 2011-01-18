#ifndef __STRUCTS_H__
#define __STRUCTS_H__

// Owned by listener
typedef struct _SRV_TRANSPORT_LISTENER {
    SRV_TRANSPORT_HANDLE pTransport;
    PLW_TASK pTask;
    PLW_TASK_GROUP pTaskGroup;
    int ListenFd;
    union
    {
        struct sockaddr_in Addr4;
#ifdef AF_INET6
        struct sockaddr_in6 Addr6;
#endif
    } Addr;
    SOCKLEN_T AddrLen;
} SRV_TRANSPORT_LISTENER, *PSRV_TRANSPORT_LISTENER;

// Top-level structure
typedef struct _SRV_TRANSPORT_HANDLE_DATA {
    SRV_TRANSPORT_PROTOCOL_DISPATCH Dispatch;
    PSRV_PROTOCOL_TRANSPORT_CONTEXT pContext;
    SRV_TRANSPORT_LISTENER Listener;
    SRV_TRANSPORT_LISTENER ListenerNetbiosSession;
    BOOL NetbiosSessionEnabled;
#ifdef AF_INET6
    SRV_TRANSPORT_LISTENER Listener6;
#endif
    PLW_THREAD_POOL pPool;
} SRV_TRANSPORT_HANDLE_DATA, *PSRV_TRANSPORT_HANDLE_DATA;

typedef ULONG SRV_SOCKET_STATE_MASK, *PSRV_SOCKET_STATE_MASK;

#define SRV_SOCKET_STATE_CLOSED         0x00000001
#define SRV_SOCKET_STATE_FD_WRITABLE    0x00000002
#define SRV_SOCKET_STATE_FD_READABLE    0x00000004
#define SRV_SOCKET_STATE_DISCONNECTED   0x00000008
#define SRV_SOCKET_STATE_USE_TIMEOUT    0x00000010

// Transport abstraction for a connection.
typedef struct _SRV_SOCKET
{
    LONG RefCount;
    LW_RTL_MUTEX Mutex;

    // Back reference.
    PSRV_TRANSPORT_LISTENER pListener;

    // Protocol connection context.
    PSRV_CONNECTION pConnection;

    // Socket-specific information.

    // Immutable for life of the task.
    int fd;
    union
    {
        struct sockaddr Addr;
        struct sockaddr_in Addr4;
#ifdef AF_INET6
        struct sockaddr_in6 Addr6;
#endif
    } ClientAddress;
    SOCKLEN_T ClientAddressLength;
    union
    {
        struct sockaddr Addr;
        struct sockaddr_in Addr4;
#ifdef AF_INET6
        struct sockaddr_in6 Addr6;
#endif
    } serverAddress;
    SOCKLEN_T serverAddressLength;
    CHAR AddressStringBuffer[SRV_SOCKET_ADDRESS_STRING_MAX_SIZE];

    PLW_TASK pTask;
    SRV_SOCKET_STATE_MASK StateMask;
    NTSTATUS DoneStatus;
    // Buffer information
    PVOID pBuffer;
    ULONG Size;
    ULONG Minimum;
    ULONG Offset;
    // Support for reading into a ZCT vector
    PLW_ZCT_VECTOR pZct;
    ULONG ZctSize;
    // Send queue - (SRV_SEND_ITEM.SendLinks)
    LW_LIST_LINKS SendHead;

    // This mutex is used for very quick operations that must not grab
    // any locks.
    LW_RTL_MUTEX TerminalMutex;

    ULONG ResetTimeoutSeconds;
    // Support for SrvSocketWakeIfNeeded - stores thread ID actively
    // being processed.
    pthread_t ProcessingThreadId;
    pthread_t* pProcessingThreadId;
} SRV_SOCKET;

typedef struct _SRV_SEND_ITEM
{
    LW_LIST_LINKS SendLinks;
    PSRV_SEND_CONTEXT pSendContext;
    PLW_ZCT_VECTOR pZct;
    PVOID pBuffer;
    ULONG Length;
    ULONG Offset;
} SRV_SEND_ITEM, *PSRV_SEND_ITEM;

typedef struct _LWIO_SRV_LISTENER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

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

    PSMB_PROD_CONS_QUEUE         pWorkQueue;

    LWIO_SRV_LISTENER            listener;

    PLWIO_PACKET_ALLOCATOR       hPacketAllocator;

    PLWIO_SRV_SHARE_ENTRY_LIST   pShareList;

} LWIO_SRV_THREADPOOL_TRANSPORT_GLOBALS, *PLWIO_SRV_THREADPOOL_TRANSPORT_GLOBALS;

#endif /* __STRUCTS_H__ */
