// Lsa.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "LsaBinding.h"


#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  reason,
                      LPVOID lpReserved)
{
	BOOL ret = TRUE;

	if (reason == DLL_PROCESS_DETACH) {
		RPC_STATUS rpcstatus = FreeLsaBinding();
		if (rpcstatus != 0) ret = FALSE;
	}

    return ret;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

