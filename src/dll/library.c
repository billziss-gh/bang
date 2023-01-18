/*
 * dll/library.c
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */

#include <dll/library.h>

CHAR ModuleFileNameA[MAX_PATH];

BOOL WINAPI DllMain(HINSTANCE Instance, DWORD Reason, PVOID Reserved)
{
    if (DetourIsHelperProcess())
        return TRUE;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileNameA(Instance, ModuleFileNameA, MAX_PATH);

        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        HookCreateProcess(TRUE, BangBeforeCreateProcessA, BangBeforeCreateProcessW);
        HookShellExecute(TRUE, BangBeforeShellExecuteA, BangBeforeShellExecuteW);
        DetourTransactionCommit();
        break;

    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        HookShellExecute(FALSE, 0, 0);
        HookCreateProcess(FALSE, 0, 0);
        DetourTransactionCommit();
        break;
    }

    return TRUE;
}
