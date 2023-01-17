/*
 * dll/library.h
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */

#ifndef BANG_DLL_LIBRARY_H_INCLUDED
#define BANG_DLL_LIBRARY_H_INCLUDED

#include <windows.h>
#include <detours.h>

extern CHAR ModuleFileNameA[];

#define MAX_COMMANDLINE                 32767

struct CreateProcessPacketA
{
    /* output; ProcessInformation must be first */
    PROCESS_INFORMATION ProcessInformation;
    /* input */
    HANDLE hToken;
    LPCSTR lpApplicationName;
    LPSTR lpCommandLine;
    LPSECURITY_ATTRIBUTES lpProcessAttributes;
    LPSECURITY_ATTRIBUTES lpThreadAttributes;
    BOOL bInheritHandles;
    DWORD dwCreationFlags;
    LPVOID lpEnvironment;
    LPCSTR lpCurrentDirectory;
    LPSTARTUPINFOA lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInformation;
    /* extra */
    BOOL Detour;
    CHAR ApplicationName[MAX_PATH];
    CHAR CommandLine[MAX_COMMANDLINE];
};

struct CreateProcessPacketW
{
    /* output; ProcessInformation must be first */
    PROCESS_INFORMATION ProcessInformation;
    /* input */
    HANDLE hToken;
    LPCWSTR lpApplicationName;
    LPWSTR lpCommandLine;
    LPSECURITY_ATTRIBUTES lpProcessAttributes;
    LPSECURITY_ATTRIBUTES lpThreadAttributes;
    BOOL bInheritHandles;
    DWORD dwCreationFlags;
    LPVOID lpEnvironment;
    LPCWSTR lpCurrentDirectory;
    LPSTARTUPINFOW lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInformation;
    LPCWSTR lpUsername;
    LPCWSTR lpDomain;
    LPCWSTR lpPassword;
    DWORD dwLogonFlags;
    /* extra */
    BOOL Detour;
    WCHAR ApplicationName[MAX_PATH];
    WCHAR CommandLine[MAX_COMMANDLINE];
};

VOID HookCreateProcess(BOOL Flag,
    VOID (*PreprocessA)(struct CreateProcessPacketA *),
    VOID (*PreprocessW)(struct CreateProcessPacketW *));

VOID BangPreprocessPacketA(struct CreateProcessPacketA *CreateProcessPacket);
VOID BangPreprocessPacketW(struct CreateProcessPacketW *CreateProcessPacket);

#endif
