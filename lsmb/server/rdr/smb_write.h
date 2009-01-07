
uint32_t
WireWriteFile(
    PSMB_TREE pTree,
    uint16_t  fid,
    uint64_t  llWriteOffset,
    uint8_t*  pWriteBuffer,
    uint16_t  wWriteLen,
    uint16_t* pwWritten,
    void*     pOverlapped
    );

