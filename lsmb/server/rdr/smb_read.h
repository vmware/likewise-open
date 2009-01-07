
uint32_t
WireReadFile(
    PSMB_TREE pTree,
    uint16_t  fid,
    uint64_t  llFileReadOffset,
    uint8_t*  pbReadBuffer,
    uint16_t  wReadLen,
    uint16_t* pwRead,
    void*     pOverlapped
    );

