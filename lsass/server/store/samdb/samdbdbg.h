#define SAMDB_DEBUG_LOG "/tmp/samdb.log"
#define SAMDB_DBG_CALL(m1, m2) SamDbDebugCall(SAMDB_DEBUG_LOG, __FUNCTION__, __FILE__, __LINE__, (m1), (m2))

void SamDbDebugCall(char *log, const char *func, char *file, int line, char *optional_msg, char *optional_data);
char *SamDbDebugWC16ToC(PWSTR pwszValue);

