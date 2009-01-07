NTSTATUS
NtCreateFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER AllocationSize,
	



NSTATUS
NtWriteFile(
	HANDLE FileHandle,
	HANDLE Event,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset,
	IN PULONG Key
	)
{
	NTSTATUS ntStatus = 0;

	return(ntStatus);
}



NTSTATUS
NtReadFile(
	OUT HANDLE FileHandle,
	IN HANDLE Event,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER  ByteOffset
	IN PULONG Key
	)
{
	NTSTATUS ntStatus = 0;

	return (ntStatus);
}

NTSTATUS
NtClose(
	IN HANDLE FileHandle
	)
{
	NTSTATUS ntStatus = 0;

	return(ntStatus);

}

NTSTATUS
NtSetFileInformation(
	IN HANDLE FileHandle
	)
{
	NTSTATUS ntStatus = 0;

	return(ntStatus);
}

NTSTATUS
NtQueryFileInformation(
	IN HANDLE FileHandle
	)
{
	NTSTATUS ntStatus = 0;

	return(ntStatus);
}	
	
