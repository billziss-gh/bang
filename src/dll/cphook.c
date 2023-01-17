/*
 * dll/cphook.c
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

#define HOOK_IMPL(N, T, A, ...)         \
    static                              \
    BOOL (WINAPI *Real_ ## N) A = N;    \
    static                              \
    BOOL WINAPI Hook_ ## N A            \
    {                                   \
        struct CreateProcessPacket ## T CreateProcessPacket;\
        BOOL Result;                    \
        __VA_ARGS__;                    \
        *lpProcessInformation = (PROCESS_INFORMATION){ 0 };\
        TlsSetValue(CreateProcessPacketTlsIndex, &CreateProcessPacket);\
        CreateProcessPacket.Detour = TRUE;\
        PreprocessPacket ## T(&CreateProcessPacket);\
        if (CreateProcessPacket.Detour) \
            Result = DetourCreateProcessWithDllEx ## T(\
                ARG(lpApplicationName), \
                ARG(lpCommandLine),     \
                ARG(lpProcessAttributes),\
                ARG(lpThreadAttributes),\
                ARG(bInheritHandles),   \
                ARG(dwCreationFlags),   \
                ARG(lpEnvironment),     \
                ARG(lpCurrentDirectory),\
                ARG(lpStartupInfo),     \
                &ARG(ProcessInformation),\
                ModuleFileNameA,        \
                Wrap_ ## N);            \
        else                            \
            Result = Wrap_ ## N(\
                ARG(lpApplicationName), \
                ARG(lpCommandLine),     \
                ARG(lpProcessAttributes),\
                ARG(lpThreadAttributes),\
                ARG(bInheritHandles),   \
                ARG(dwCreationFlags),   \
                ARG(lpEnvironment),     \
                ARG(lpCurrentDirectory),\
                ARG(lpStartupInfo),     \
                &ARG(ProcessInformation));\
        TlsSetValue(CreateProcessPacketTlsIndex, 0);\
        if (Result)                     \
            *lpProcessInformation = ARG(ProcessInformation);\
        return Result;                  \
    }
#define HOOK_ATTACH(F, N)               \
    (((F) ? DetourAttach : DetourDetach)((PVOID *)&Real_ ## N, Hook_ ## N))

#define ARG(N)                          \
    CreateProcessPacket.N
#define SETARG(N)                       \
    CreateProcessPacket.N = N

#define Wrap_CreateProcessA             Real_CreateProcessA
#define Wrap_CreateProcessW             Real_CreateProcessW
static
BOOL WINAPI Wrap_CreateProcessAsUserA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);
static
BOOL WINAPI Wrap_CreateProcessAsUserW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);
static
BOOL WINAPI Wrap_CreateProcessWithLogonW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);
static
BOOL WINAPI Wrap_CreateProcessWithTokenW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);

static DWORD CreateProcessPacketTlsIndex = TLS_OUT_OF_INDEXES;
static VOID (*PreprocessPacketA)(struct CreateProcessPacketA *);
static VOID (*PreprocessPacketW)(struct CreateProcessPacketW *);

HOOK_IMPL(CreateProcessA, A, (
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(lpProcessAttributes);
    SETARG(lpThreadAttributes);
    SETARG(bInheritHandles);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

HOOK_IMPL(CreateProcessW, W, (
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(lpProcessAttributes);
    SETARG(lpThreadAttributes);
    SETARG(bInheritHandles);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

HOOK_IMPL(CreateProcessAsUserA, A, (
    HANDLE hToken,
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(hToken);
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(lpProcessAttributes);
    SETARG(lpThreadAttributes);
    SETARG(bInheritHandles);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

HOOK_IMPL(CreateProcessAsUserW, W, (
    HANDLE hToken,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(hToken);
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(lpProcessAttributes);
    SETARG(lpThreadAttributes);
    SETARG(bInheritHandles);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

HOOK_IMPL(CreateProcessWithLogonW, W, (
    LPCWSTR lpUsername,
    LPCWSTR lpDomain,
    LPCWSTR lpPassword,
    DWORD dwLogonFlags,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(lpUsername);
    SETARG(lpDomain);
    SETARG(lpPassword);
    SETARG(dwLogonFlags);
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

HOOK_IMPL(CreateProcessWithTokenW, W, (
    HANDLE hToken,
    DWORD dwLogonFlags,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation),
{
    SETARG(hToken);
    SETARG(dwLogonFlags);
    SETARG(lpApplicationName);
    SETARG(lpCommandLine);
    SETARG(dwCreationFlags);
    SETARG(lpEnvironment);
    SETARG(lpCurrentDirectory);
    SETARG(lpStartupInfo);
    SETARG(lpProcessInformation);
})

static
BOOL WINAPI Wrap_CreateProcessAsUserA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    struct CreateProcessPacketA *CreateProcessPacket = (PVOID)lpProcessInformation;
    if (TlsGetValue(CreateProcessPacketTlsIndex) == CreateProcessPacket)
        return Real_CreateProcessAsUserA(
            CreateProcessPacket->hToken,
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
    else
        return Real_CreateProcessA(
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
}

static
BOOL WINAPI Wrap_CreateProcessAsUserW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    struct CreateProcessPacketW *CreateProcessPacket = (PVOID)lpProcessInformation;
    if (TlsGetValue(CreateProcessPacketTlsIndex) == CreateProcessPacket)
        return Real_CreateProcessAsUserW(
            CreateProcessPacket->hToken,
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
    else
        return Real_CreateProcessW(
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
}

static
BOOL WINAPI Wrap_CreateProcessWithLogonW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    struct CreateProcessPacketW *CreateProcessPacket = (PVOID)lpProcessInformation;
    if (TlsGetValue(CreateProcessPacketTlsIndex) == CreateProcessPacket)
        return Real_CreateProcessWithLogonW(
            CreateProcessPacket->lpUsername,
            CreateProcessPacket->lpDomain,
            CreateProcessPacket->lpPassword,
            CreateProcessPacket->dwLogonFlags,
            lpApplicationName,
            lpCommandLine,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
    else
        return Real_CreateProcessW(
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
}

static
BOOL WINAPI Wrap_CreateProcessWithTokenW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    struct CreateProcessPacketW *CreateProcessPacket = (PVOID)lpProcessInformation;
    if (TlsGetValue(CreateProcessPacketTlsIndex) == CreateProcessPacket)
        return Real_CreateProcessWithTokenW(
            CreateProcessPacket->hToken,
            CreateProcessPacket->dwLogonFlags,
            lpApplicationName,
            lpCommandLine,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
    else
        return Real_CreateProcessW(
            lpApplicationName,
            lpCommandLine,
            lpProcessAttributes,
            lpThreadAttributes,
            bInheritHandles,
            dwCreationFlags,
            lpEnvironment,
            lpCurrentDirectory,
            lpStartupInfo,
            lpProcessInformation);
}

VOID HookCreateProcess(BOOL Flag,
    VOID (*PreprocessA)(struct CreateProcessPacketA *),
    VOID (*PreprocessW)(struct CreateProcessPacketW *))
{
    if (Flag)
        CreateProcessPacketTlsIndex = TlsAlloc();
    if (Flag)
    {
        PreprocessPacketA = PreprocessA;
        PreprocessPacketW = PreprocessW;
    }

    if (TLS_OUT_OF_INDEXES != CreateProcessPacketTlsIndex)
    {
        HOOK_ATTACH(Flag, CreateProcessA);
        HOOK_ATTACH(Flag, CreateProcessW);
        HOOK_ATTACH(Flag, CreateProcessAsUserA);
        HOOK_ATTACH(Flag, CreateProcessAsUserW);
        HOOK_ATTACH(Flag, CreateProcessWithLogonW);
        HOOK_ATTACH(Flag, CreateProcessWithTokenW);
    }

    if (!Flag)
    {
        PreprocessPacketA = 0;
        PreprocessPacketW = 0;
    }
    if (!Flag && TLS_OUT_OF_INDEXES != CreateProcessPacketTlsIndex)
        TlsFree(CreateProcessPacketTlsIndex);
}
