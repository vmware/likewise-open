#define SAMDB_DEBUG_LOG "/tmp/samdb.log"
#define SAMDB_DBG_CALL SamDbDebugCall(SAMDB_DEBUG_LOG, __FILE__, __LINE__)

void SamDbDebugCall(char *log, char *file, int line);
