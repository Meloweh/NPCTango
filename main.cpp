#include "header.h"

bool WINAPI DllMain(HINSTANCE hinstdll, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		ExtraThread *et = new ExtraThread();
		CreateThread(0, 0, ExtraThread::ThreadFunc, et, 0L, 0);
	}
	else return false;

	return true;
}
