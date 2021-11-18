// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern "C" __declspec(dllexport) bool X();
extern "C" __declspec(dllexport) bool Y();

bool X()
{
	return true;
}

bool Y()
{
	return true;
}

bool APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		LoadLibraryW(L"Deftones.dll");
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return true;
}