
NTSTATUS
SrvDeviceCreate(
    IO_DEVICE_HANDLE hDevice,
    PIRP      pIrp
    );

NTSTATUS
SrvDeviceClose(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceRead(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceWrite(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceControlIO(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceControlFS(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceFlush(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceQueryInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceSetInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );
