#ifndef __SRV_DEVICE_IO_H__
#define __SRV_DEVICE_IO_H__

NTSTATUS
SrvDeviceControlIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    );

#endif /* __SRV_DEVICE_IO_H__ */

