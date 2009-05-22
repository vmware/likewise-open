#ifndef __SRV_PATH_H__
#define __SRV_PATH_H__

NTSTATUS
SrvBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    );

#endif /* __SRV_PATH_H__ */
