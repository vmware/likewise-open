

DWORD
DirectoryOpenHandle(
	LPWSTR pszDirectoryName,
	HANDLE * phDirectory
	)
{
	DWORD dwError = 0;

	ENTER_CRITICAL_SECTION();

	dwError = FindTrustedDirectory(pszDirectory, &pDirectoryEntry);
	BAIL_ON_ERROR(dwError);

	dwError = AllocateDirectoryHandle(
					&pDirectoryHandle
					);
	BAIL_ON_ERROR(dwError);



	return dwError;
}



VOID
DirectoryCloseHandle(
	HANDLE hDirectory
	)
{


}