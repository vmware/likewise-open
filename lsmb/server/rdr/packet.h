
/* @todo: support AndX */
/* @todo: support signing */
DWORD
SMBPacketMarshallHeader(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    uint8_t     command,
    uint32_t    error,
    uint32_t    isResponse,
    uint16_t    tid,
    uint32_t    pid,
    uint16_t    uid,
    uint16_t    mid,
    BOOLEAN     bSignMessages,
    PSMB_PACKET pPacket
    );

DWORD
SMBPacketMarshallFooter(
    PSMB_PACKET pPacket
    );

BOOLEAN
SMBPacketIsSigned(
    PSMB_PACKET pPacket
    );

DWORD
SMBPacketVerifySignature(
    PSMB_PACKET pPacket,
    DWORD       dwExpectedSequence,
    PBYTE       pSessionKey,
    DWORD       dwSessionKeyLength
    );

DWORD
SMBPacketSign(
    PSMB_PACKET pPacket,
    DWORD       dwSequence,
    PBYTE       pSessionKey,
    DWORD       dwSessionKeyLength
    );

DWORD
SMBPacketSend(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

DWORD
SMBPacketReceiveAndUnmarshall(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

