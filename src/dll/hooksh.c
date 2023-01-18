/*
 * dll/hooksh.c
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

#define HOOK_IMPL(N, T, A)              \
    static                              \
    HINSTANCE (WINAPI *Real_ ## N) A = 0;\
    static                              \
    HINSTANCE WINAPI Hook_ ## N A       \
    {                                   \
        SHELLEXECUTEINFO ## T ShellExecuteInfo =\
        {                               \
            .cbSize = sizeof ShellExecuteInfo,\
            .fMask = SEE_MASK_FLAG_NO_UI,\
            .hwnd = hwnd,               \
            .lpVerb = lpOperation,      \
            .lpFile = lpFile,           \
            .lpParameters = lpParameters,\
            .lpDirectory = lpDirectory, \
            .nShow = nShowCmd,          \
        };                              \
        BeforeShellExecute ## T(&ShellExecuteInfo);\
        Real_ShellExecuteEx ## T(&ShellExecuteInfo);\
        return ShellExecuteInfo.hInstApp;\
    }
#define HOOK_IMPL_EX(N, T, A)           \
    static                              \
    BOOL (WINAPI *Real_ ## N) A = 0;\
    static                              \
    BOOL WINAPI Hook_ ## N A       \
    {                                   \
        SHELLEXECUTEINFO ## T ShellExecuteInfo = *pExecInfo;\
        BOOL Result;                    \
        BeforeShellExecute ## T(&ShellExecuteInfo);\
        Result = Real_ShellExecuteEx ## T(&ShellExecuteInfo);\
        pExecInfo->hInstApp = ShellExecuteInfo.hInstApp;\
        pExecInfo->hProcess = ShellExecuteInfo.hProcess;\
        return Result;                  \
    }
#define HOOK_ATTACH(F, N)               \
    (((F) ? DetourAttach : DetourDetach)((PVOID *)&Real_ ## N, Hook_ ## N))
#define HOOK_GETPROC(M, N)              \
    (Real_ ## N = (PVOID)GetProcAddress(M, #N))

VOID (*BeforeShellExecuteA)(SHELLEXECUTEINFOA *ShellExecuteInfo);
VOID (*BeforeShellExecuteW)(SHELLEXECUTEINFOW *ShellExecuteInfo);

HOOK_IMPL_EX(ShellExecuteExA, A, (
    SHELLEXECUTEINFOA *pExecInfo))

HOOK_IMPL_EX(ShellExecuteExW, W, (
    SHELLEXECUTEINFOW *pExecInfo))

HOOK_IMPL(ShellExecuteA, A, (
    HWND hwnd,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPCSTR lpParameters,
    LPCSTR lpDirectory,
    INT nShowCmd))

HOOK_IMPL(ShellExecuteW, W, (
    HWND hwnd,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT nShowCmd))

VOID HookShellExecute(BOOL Flag,
    VOID (*BeforeA)(SHELLEXECUTEINFOA *),
    VOID (*BeforeW)(SHELLEXECUTEINFOW *))
{
    static HANDLE Module;

    if (Flag)
    {
        Module = GetModuleHandleA("shell32.dll");
        if (0 != Module)
        {
            HOOK_GETPROC(Module, ShellExecuteExA);
            HOOK_GETPROC(Module, ShellExecuteExW);
            HOOK_GETPROC(Module, ShellExecuteA);
            HOOK_GETPROC(Module, ShellExecuteW);
        }
    }
    if (Flag)
    {
        BeforeShellExecuteA = BeforeA;
        BeforeShellExecuteW = BeforeW;
    }

    if (0 != Module)
    {
        HOOK_ATTACH(Flag, ShellExecuteExA);
        HOOK_ATTACH(Flag, ShellExecuteExW);
        HOOK_ATTACH(Flag, ShellExecuteA);
        HOOK_ATTACH(Flag, ShellExecuteW);
    }

    if (!Flag)
    {
        BeforeShellExecuteA = 0;
        BeforeShellExecuteW = 0;
    }
}
